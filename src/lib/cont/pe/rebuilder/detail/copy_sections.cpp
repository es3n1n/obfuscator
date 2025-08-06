#include "cont/pe/rebuilder/rebuilder.hpp"

#include <magic_enum.hpp>

namespace cont::pe::detail {
    namespace {
        win::section_header_t assemble_section_header(const Section& section) {
            win::section_header_t result{};

            std::ranges::copy(section.name, reinterpret_cast<char*>(result.name.short_name));
            result.virtual_size = static_cast<std::uint32_t>(section.virtual_size);
            result.virtual_address = static_cast<std::uint32_t>(section.virtual_address);
            result.size_raw_data = static_cast<std::uint32_t>(section.size_raw_data);
            result.ptr_raw_data = static_cast<std::uint32_t>(section.ptr_raw_data);

            /// \fixme @es3n1n: This looks ugly
            result.characteristics.cnt_code = section.characteristics.cnt_code;
            result.characteristics.cnt_init_data = section.characteristics.cnt_init_data;
            result.characteristics.cnt_uninit_data = section.characteristics.cnt_uninit_data;
            result.characteristics.lnk_info = section.characteristics.lnk_info;
            result.characteristics.lnk_remove = section.characteristics.lnk_remove;
            result.characteristics.lnk_comdat = section.characteristics.lnk_comdat;
            result.characteristics.no_defer_spec_exc = section.characteristics.no_defer_spec_exc;
            result.characteristics.mem_far = section.characteristics.mem_far;
            result.characteristics.mem_purgeable = section.characteristics.mem_purgeable;
            result.characteristics.mem_locked = section.characteristics.mem_locked;
            result.characteristics.mem_preload = section.characteristics.mem_preload;
            result.characteristics.alignment = section.characteristics.alignment;
            result.characteristics.lnk_nreloc_ovfl = section.characteristics.lnk_nreloc_ovfl;
            result.characteristics.mem_discardable = section.characteristics.mem_discardable;
            result.characteristics.mem_not_cached = section.characteristics.mem_not_cached;
            result.characteristics.mem_not_paged = section.characteristics.mem_not_paged;
            result.characteristics.mem_shared = section.characteristics.mem_shared;
            result.characteristics.mem_execute = section.characteristics.mem_execute;
            result.characteristics.mem_read = section.characteristics.mem_read;
            result.characteristics.mem_write = section.characteristics.mem_write;
            return result;
        }

        template <AnyRawImage Img>
        void copy_sections_(Image* image, std::vector<std::uint8_t>& data) {
            /// Casting our buffer as the raw image
            auto* out_img = reinterpret_cast<Img*>(data.data());
            auto* nt_headers = out_img->get_nt_headers();
            auto* file_header = &nt_headers->file_header;
            auto* optional_header = &nt_headers->optional_header;

            /// Obtaining sections pointer within the nt headers
            auto* sections = out_img->get_nt_headers()->get_sections();

            /// Raligning sections
            image->realign_sections();

            /// Validating that there's enough space for our sections
            const auto sections_start = memory::address{sections};
            auto header_end = memory::address{out_img}.offset(optional_header->size_headers);
            if (sections_start + (sizeof(win::section_header_t) * (image->sections.size() + 1)) > header_end) [[unlikely]] {
                throw std::runtime_error("pe: rebuilder: unable to fit new sections");
            }

            /// Erasing previous data directories info
            std::memset(&optional_header->data_directories, 0, sizeof(optional_header->data_directories));

            /// Iterating over the max value of sections
            /// If the amount of our sections is less than the number of sections
            /// within the PE that we're rebuilding, we would essentially need to
            /// erase all the other sections that presents within the PE
            for (std::size_t i = 0; i < std::max(static_cast<std::size_t>(file_header->num_sections), image->sections.size()); ++i) {
                /// Erasing previous data first
                std::memset(&sections[i], 0, sizeof(win::section_header_t));

                /// If we only need to erase the prev section data
                if (i >= image->sections.size()) {
                    continue;
                }

                /// Obtaining our section info
                auto& section = image->sections.at(i);

                /// Assembling the new section header and copying it
                auto sec_header = assemble_section_header(section);
                if (auto write_res = memory::address{&sections[i]}.write(memory::address{&sec_header}.as<const void*>(), sizeof(win::section_header_t));
                    !write_res.has_value()) {
                    throw std::runtime_error("pe: rebuilder: unable to write section header");
                }

                /// Copying section data, if needed
                if (!section.raw_data.empty()) {
                    auto sec_ptr = memory::address{data.data()}.offset(section.ptr_raw_data);
                    if (auto write_res = sec_ptr.write(section.raw_data.data(), section.raw_data.size()); !write_res.has_value()) {
                        throw std::runtime_error("pe: rebuilder: unable to write section raw data");
                    }
                }

                /// Updating the data directories
                for (const auto dir_type : magic_enum::enum_values<DirectoryType>()) {
                    if (dir_type == DirectoryType::MAX_LENGTH) {
                        continue;
                    }
                    const auto props = section.directory_info(dir_type);
                    if (!props.has_value()) {
                        continue;
                    }

                    win::data_directory_t dir_hdr = {
                        .rva = static_cast<std::uint32_t>(section.virtual_address + props->offset),
                        .size = static_cast<std::uint32_t>(props->size),
                    };

                    switch (dir_type) {
                    case DirectoryType::Export:
                        optional_header->data_directories.export_directory = dir_hdr;
                        break;
                    case DirectoryType::Import:
                        optional_header->data_directories.import_directory = dir_hdr;
                        break;
                    case DirectoryType::Resource:
                        optional_header->data_directories.resource_directory = dir_hdr;
                        break;
                    case DirectoryType::Exception:
                        optional_header->data_directories.exception_directory = dir_hdr;
                        break;
                    case DirectoryType::Security:
                        optional_header->data_directories.security_directory = win::raw_data_directory_t{
                            .ptr_raw_data = dir_hdr.rva,
                            .size = dir_hdr.size,
                        };
                        break;
                    case DirectoryType::Reloc:
                        optional_header->data_directories.basereloc_directory = dir_hdr;
                        break;
                    case DirectoryType::Debug:
                        optional_header->data_directories.debug_directory = dir_hdr;
                        break;
                    case DirectoryType::Architecture:
                        if constexpr (!std::is_same_v<Img, win::image_x86_t>) {
                            optional_header->data_directories.architecture_directory = dir_hdr;
                        }
                        break;
                    case DirectoryType::GlobalPtr:
                        optional_header->data_directories.globalptr_directory = dir_hdr;
                        break;
                    case DirectoryType::Tls:
                        optional_header->data_directories.tls_directory = dir_hdr;
                        break;
                    case DirectoryType::LoadConfig:
                        optional_header->data_directories.load_config_directory = dir_hdr;
                        break;
                    case DirectoryType::BoundImport:
                        optional_header->data_directories.bound_import_directory = dir_hdr;
                        break;
                    case DirectoryType::Iat:
                        optional_header->data_directories.iat_directory = dir_hdr;
                        break;
                    case DirectoryType::DelayImport:
                        optional_header->data_directories.delay_import_directory = dir_hdr;
                        break;
                    case DirectoryType::ComDescriptor:
                        optional_header->data_directories.com_descriptor_directory = dir_hdr;
                        break;
                    default:
                        throw std::out_of_range("pe: rebuilder: unsupported directory type");
                    }
                }
            }

            /// Updating the sections count
            file_header->num_sections = static_cast<std::uint16_t>(image->sections.size());
        }
    } // namespace

    void copy_sections(Image* image, std::vector<std::uint8_t>& data) {
        switch (image->mode()) {
        case ImageMode::X64: {
            copy_sections_<win::image_x64_t>(image, data);
            break;
        }
        case ImageMode::X86: {
            copy_sections_<win::image_x86_t>(image, data);
            break;
        }
        default:
            throw std::out_of_range("cont::pe::detail::copy_sections: Unsupported image mode");
        }
    }
} // namespace cont::pe::detail