#include "pe/rebuilder/rebuilder.hpp"
#include "util/format.hpp"

#include <list>

namespace pe::detail {
    namespace {
        constexpr std::size_t kRelocBlockAlignment = 0x1000;

        // Erasing previous relocations from the binary
        //
        template <any_image_t Img>
        void erase_relocations(Img* image) {
            // Looking for the section that contains relocations
            //
            auto reloc_section = std::ranges::find_if(image->sections, [](const section_t& sec) -> bool { //
                return sec.contains_dir.reloc.has_value();
            });

            // No relocation dir?
            //
            if (reloc_section == std::end(image->sections)) {
                return;
            }

            // Obtaining reloc entry offset from the base of section
            //
            auto reloc_offset = image->raw_image->get_directory(win::directory_id::directory_entry_basereloc)->rva;
            reloc_offset -= reloc_section->virtual_address;

            // Obtaining reloc directory and iterating over blocks in order to get the last block
            //
            auto* dir = memory::cast<win::reloc_directory_t*>(reloc_section->raw_data.data() + reloc_offset);
            auto* block = &dir->first_block;
            for (; block && block->size_block != 0U; block = block->next()) {
                // do nothing
            }

            // Calculating reloc dir size
            //
            const auto dir_size = reinterpret_cast<uintptr_t>(block) - reinterpret_cast<uintptr_t>(dir);

            // Erasing current reloc info
            //
            std::memset(memory::cast<void*>(dir), 0x00, dir_size);

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
        template <any_image_t Img>
        void assemble_relocations(Img* image) {
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
            std::unordered_map<memory::address, std::list<relocation_t>> blocks;
            std::size_t section_size = 0ULL;

            // Iterating over relocations and obtaining start RVAs,
            // Estimating section size
            //
            for (auto&& [rva, relocation] : image->relocations) {
                const auto aligned_rva = rva.align_down(kRelocBlockAlignment);

                // Accounting new block header if we're creating one
                //
                if (!blocks.contains(aligned_rva)) {
                    section_size += sizeof(win::reloc_block_t);
                }

                // Prepending relocation to the block
                //
                blocks[aligned_rva].emplace_back(std::move(relocation));

                // Accounting entry
                //
                section_size += sizeof(win::reloc_entry_t);
            }

            // Obtaining a pointer to the directory header
            //
            auto* dir_header = image->get_directory(win::directory_id::directory_entry_basereloc);
            if (dir_header == nullptr) {
                throw std::runtime_error("pe: rebuilder: .reloc header not found");
            }

            // Inserting the new section with our relocations
            //
            auto& new_section = image->new_section(sections::e_section_t::RELOC, section_size);
            auto section_data = memory::address{new_section.raw_data.data()};
            auto section_end = section_data.offset(new_section.raw_data.size());

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
                        .type = relocation.type,
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
            new_section.set_contained_dir(win::directory_id::directory_entry_basereloc, 0, section_size);
        }

        template <any_image_t Img>
        void update_relocations_(Img* image, std::vector<std::uint8_t>& data [[maybe_unused]]) {
            erase_relocations(image);
            assemble_relocations(image);
        }
    } // namespace

    void update_relocations(const ImgWrapped image, std::vector<std::uint8_t>& data) {
        return UNWRAP_IMAGE(void, update_relocations_);
    }
} // namespace pe::detail