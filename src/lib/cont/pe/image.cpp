//
// Created by es3n1n on 2025-08-06.
//
#include "cont/pe/image.hpp"

#include "cont/pe/rebuilder/rebuilder.hpp"
#include "magic_enum.hpp"

namespace cont::pe {
    [[nodiscard]] ImageMode Image::mode() const {
        switch (x86()->get_file_header()->machine) {
        case win::machine_id::amd64:
            return ImageMode::X64;
        case win::machine_id::i386:
            return ImageMode::X86;
        default:
            throw std::runtime_error("cont::pe: Unsupported machine type");
        }
    }

    [[nodiscard]] bool Image::verify_integrity() const {
        return x86()->dos_header.e_magic == win::DOS_HDR_MAGIC;
    }

    [[nodiscard]] std::size_t Image::get_image_base() const {
        switch (mode()) {
        case ImageMode::X64: {
            return x64()->get_nt_headers()->optional_header.image_base;
        }
        case ImageMode::X86: {
            return x86()->get_nt_headers()->optional_header.image_base;
        }
        default:
            throw std::out_of_range("cont::pe::image_base: Unsupported image mode");
        }
    }

    [[nodiscard]] std::size_t Image::get_section_alignment() const {
        switch (mode()) {
        case ImageMode::X64: {
            return x64()->get_nt_headers()->optional_header.section_alignment;
        }
        case ImageMode::X86: {
            return x86()->get_nt_headers()->optional_header.section_alignment;
        }
        default:
            throw std::out_of_range("cont::pe::get_section_alignment: Unsupported image mode");
        }
    }

    [[nodiscard]] std::size_t Image::get_file_alignment() const {
        switch (mode()) {
        case ImageMode::X64: {
            return x64()->get_nt_headers()->optional_header.file_alignment;
        }
        case ImageMode::X86: {
            return x86()->get_nt_headers()->optional_header.file_alignment;
        }
        default:
            throw std::out_of_range("cont::pe::file_alignment: Unsupported image mode");
        }
    }

    [[nodiscard]] std::size_t Image::get_base_of_code() const {
        switch (mode()) {
        case ImageMode::X64: {
            return x64()->get_nt_headers()->optional_header.base_of_code;
        }
        case ImageMode::X86: {
            return x86()->get_nt_headers()->optional_header.base_of_code;
        }
        default:
            throw std::out_of_range("cont::pe::base_of_code: Unsupported image mode");
        }
    }

    [[nodiscard]] std::vector<std::uint8_t> Image::rebuild_image() {
        auto ctx = RebuilderContext{.image = this};
        return rebuild_pe(ctx);
    }

    void Image::realign_sections() {
        /// Nothing to realign
        if (sections.size() <= 1) {
            return;
        }

        /// Making sure that all section virtual sizes are aligned
        for (std::size_t i = 0; i < sections.size() - 1; ++i) {
            auto& sec = sections.at(i);
            const auto& next_sec = sections.at(i + 1);

            sec.virtual_size = next_sec.virtual_address - sec.virtual_address;
        }
    }

    void Image::update_sections() {
        const auto proceed = [this]<typename Ty>(const Ty* raw_image) -> void {
            // Obtaining stuff that would be needed
            //
            const auto* nt_hdr = raw_image->get_nt_headers();

            // Reserving the num of sections
            //
            sections.clear();
            sections.reserve(nt_hdr->file_header.num_sections);

            for (std::size_t i = 0; i < nt_hdr->file_header.num_sections; ++i) {
                // Getting a section and validating it
                //
                const win::section_header_t* section = nt_hdr->get_section(i);
                if (section == nullptr) {
                    continue;
                }

                // Inserting a section to the result array
                //
                auto& new_elem = sections.emplace_back(to_cont(*section));
                new_elem.raw_data.resize(new_elem.size_raw_data, 0);

                // NOLINTNEXTLINE
                std::memcpy(new_elem.raw_data.data(), reinterpret_cast<std::uint8_t*>(reinterpret_cast<uintptr_t>(raw_image) + new_elem.ptr_raw_data), //
                            new_elem.size_raw_data);
            }

            // Signalising that we successfully parsed sections
            //
            logger::debug("pe: parsed {} sections", sections.size());

            // Sort by virtual address
            //
            const struct {
                bool operator()(const Section& lhs, const Section& rhs) const {
                    return lhs.virtual_address < rhs.virtual_address;
                }
            } comp;
            std::sort(sections.begin(), sections.end(), comp);

            // Marking directories
            //
            for (auto dir_id : magic_enum::enum_values<win::directory_id>()) {
                // Trying to find the header of the directory, skipping if not present
                //
                auto dir_hdr = raw_image->get_directory(dir_id);
                if (!dir_hdr || !dir_hdr->present()) {
                    continue;
                }

                // Looking up the section by rva and changing flag
                //
                auto* sec = rva_to_section(dir_hdr->rva);
                sec->set_contained_dir(to_cont(dir_id), dir_hdr->rva - sec->virtual_address, dir_hdr->size);
            }
        };

        switch (mode()) {
        case ImageMode::X64:
            proceed(x64());
            break;
        case ImageMode::X86:
            proceed(x86());
            break;
        default:
            throw std::out_of_range("cont::pe::update_sections: Unsupported image mode");
        }
    }

    void Image::update_relocations() {
        const auto proceed = [this]<typename Ty>(const Ty* raw_image) -> void {
            // Obtaining a pointer to reloc directory header
            //
            const win::data_directory_t* reloc_hdr = raw_image->get_directory(win::directory_id::directory_entry_basereloc);
            if ((reloc_hdr == nullptr) || !reloc_hdr->present()) [[unlikely]] {
                logger::warn("pe: relocation directory header does not present?");
                return;
            }

            // Obtaining a pointer to reloc directory
            //
            const win::reloc_directory_t* base_reloc = rva_to_ptr<win::reloc_directory_t>(reloc_hdr->rva);
            const auto reloc_size = ptr_size();

            // Iterating over reloc blocks
            //
            for (const auto* reloc_block = &base_reloc->first_block; //
                 (reloc_block != nullptr) && (reloc_block->size_block != 0U) && (reloc_block->base_rva != 0U); //
                 reloc_block = reloc_block->next()) {
                // Iterating over reloc entries
                //
                for (const auto& [offset, type] : *reloc_block) {
                    // Skip ignored relocations
                    // \todo @es3n1n: remove me
                    if (type == win::reloc_type_id::rel_based_absolute) {
                        continue;
                    }

                    // Inserting parsed reloc data
                    //
                    auto rva = static_cast<memory::address>(reloc_block->base_rva) + memory::address(offset);

                    // Just to be sure
                    //
                    if (relocations.contains(rva)) [[unlikely]] {
                        throw std::runtime_error(std::format("pe: duplicated {:#x} rva entry", rva));
                    }

                    // Inserting relocation info
                    //
                    relocations[rva] = Relocation{
                        .rva = rva, //
                        .size = static_cast<std::uint8_t>(reloc_size), //
                        .type = static_cast<RelocationType>(type), //
                    };
                }
            }

            logger::debug("pe: parsed total number of {} relocations", relocations.size());
        };

        switch (mode()) {
        case ImageMode::X64:
            proceed(x64());
            break;
        case ImageMode::X86:
            proceed(x86());
            break;
        default:
            throw std::out_of_range("cont::pe::update_relocations: Unsupported image mode");
        }
    }

    [[nodiscard]] win::data_directory_t* Image::get_directory(win::directory_id dir_id) {
        const auto proceed = [dir_id]<AnyRawImage Ty>(Ty* raw_image) -> win::data_directory_t* {
            auto nt_hdrs = raw_image->get_nt_headers();
            if (nt_hdrs->optional_header.num_data_directories <= dir_id) {
                return nullptr;
            }

            return &nt_hdrs->optional_header.data_directories.entries[dir_id];
        };

        switch (mode()) {
        case ImageMode::X64: {
            return proceed(x64());
        }
        case ImageMode::X86: {
            return proceed(x86());
        }
        default:
            throw std::out_of_range("cont::pe::get_directory: Unsupported image mode");
        }
    }
} // namespace cont::pe