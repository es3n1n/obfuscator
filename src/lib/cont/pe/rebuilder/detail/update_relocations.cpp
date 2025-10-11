#include "cont/pe/rebuilder/rebuilder.hpp"
#include "util/format.hpp"

#include <list>

namespace cont::pe::detail {
    namespace {
        constexpr std::size_t kRelocBlockAlignment = 0x1000;

        // Erasing previous relocations from the binary
        //
        template <AnyRawImage Img>
        void erase_relocations(Image* image) {
            // Looking for the section that contains relocations
            //
            auto reloc_section = std::ranges::find_if(image->sections, [](const Section& sec) -> bool { //
                return sec.directory_info(DirectoryType::Reloc).has_value();
            });

            // No relocation dir?
            //
            if (reloc_section == std::end(image->sections)) {
                return;
            }

            // Obtaining reloc entry offset from the base of section
            //
            auto reloc_offset = image->raw_image().ptr<Img>()->get_directory(win::directory_id::directory_entry_basereloc)->rva;
            reloc_offset -= static_cast<std::uint32_t>(reloc_section->virtual_address);

            // Obtaining reloc directory and iterating over blocks in order to get the last block
            //
            auto* dir = reinterpret_cast<win::reloc_directory_t*>(reloc_section->raw_data.data() + reloc_offset);
            auto* block = &dir->first_block;
            for (; block != nullptr && block->base_rva != 0U && block->size_block != 0U; block = block->next()) {
                // do nothing
            }

            // Calculating reloc dir size
            //
            const auto dir_size = reinterpret_cast<uintptr_t>(block) - reinterpret_cast<uintptr_t>(dir);

            // Erasing current reloc info
            //
            std::memset(dir, 0x00, dir_size);

            // If this section consists only of zeroes, then we could just remove the whole section :)
            // Otherwise, we should keep it.
            //
            if (std::ranges::find_if(reloc_section->raw_data, [](const uint8_t val) -> bool { //
                    return val != 0;
                }) != reloc_section->raw_data.end()) {
                return;
            }

            // Erasing section yay
            //
            image->sections.erase(reloc_section);
            logger::debug("pe: erased the whole reloc section :thinking:");
        }

        // Assembling the new reloc section
        //
        void assemble_relocations(Image* image) {
            // No relocations?
            //
            if (image->relocations.empty()) [[unlikely]] {
                logger::info("pe: rebuilder: skipped .reloc section assembling");
                return;
            }

            // Nicely assembling blocks
            // key is start rva, values are relocations, e.g.:
            // 0x1000 ->
            //	relocation {rva=0x1010}
            //	relocation {rva=0x1111}
            // 0x3000 ->
            //	relocation {rva=0x3FFFF}
            // etc
            //
            std::unordered_map<memory::address, std::list<Relocation>> blocks;
            auto section_header = sections::get(sections::e_section_t::RELOC);

            // Iterating over relocations and obtaining start RVAs,
            // Estimating section size
            //
            for (auto&& [rva, relocation] : image->relocations) {
                const auto aligned_rva = rva.align_down(kRelocBlockAlignment);

                // Accounting new block header if we're creating one
                //
                if (!blocks.contains(aligned_rva)) {
                    section_header.size_raw_data += sizeof(win::reloc_block_t);
                }

                // Prepending relocation to the block
                //
                blocks[aligned_rva].emplace_back(relocation);

                // Accounting entry
                //
                section_header.size_raw_data += sizeof(win::reloc_entry_t);
            }

            // Obtaining a pointer to the directory header
            //
            if (const auto* dir_header = image->get_directory(win::directory_id::directory_entry_basereloc); dir_header == nullptr) {
                throw std::runtime_error("pe: rebuilder: .reloc header not found");
            }

            // Inserting the new section with our relocations
            //
            auto& new_section = image->new_section(section_header);
            auto section_data = memory::address{new_section.raw_data.data()};
            const auto section_end = section_data.offset(static_cast<std::ptrdiff_t>(new_section.raw_data.size()));

            // Serializing reloc entries
            //
            for (auto&& [rva, relocations] : blocks) {
                // Assembling block header
                //
                auto* header = section_data.self_inc_ptr<win::reloc_block_t>();
                header->base_rva = rva.as<std::uint32_t>();
                header->size_block = static_cast<uint32_t>(relocations.size() * sizeof(win::reloc_entry_t)) + sizeof(win::reloc_block_t);

                // Serializing entries
                //
                for (auto&& relocation : relocations) {
                    // Sanity checks
                    //
                    if (relocation.rva < rva || section_data >= section_end) {
                        throw std::runtime_error("pe: rebuilder: reloc serializer sanity error");
                    }

                    // Encoding our relocation struct to the windows' one
                    //
                    const auto reloc_encoded = win::reloc_entry_t{
                        .offset = (relocation.rva - rva).as<uint16_t>(),
                        .type = static_cast<win::reloc_type_id>(relocation.type),
                    };

                    // Writing it
                    //
                    if (auto result = section_data.self_write_inc(reloc_encoded); !result.has_value()) {
                        throw std::runtime_error("pe: rebuilder: Unable to write reloc");
                    }
                }
            }

            // Mark as sec with relocs
            //
            new_section.set_contained_dir(DirectoryType::Reloc, 0, new_section.size_raw_data);
        }

        template <AnyRawImage Img>
        void update_relocations_(Image* image) {
            erase_relocations<Img>(image);
            assemble_relocations(image);
        }
    } // namespace

    void update_relocations(Image* image, const std::vector<std::uint8_t>& data) {
        std::ignore = data;
        switch (image->mode()) {
        case ImageMode::X64: {
            update_relocations_<win::image_x64_t>(image);
            break;
        }
        case ImageMode::X86: {
            update_relocations_<win::image_x86_t>(image);
            break;
        }
        default:
            throw std::out_of_range("cont::pe::detail::update_relocations: Unsupported image mode");
        }
    }
} // namespace cont::pe::detail