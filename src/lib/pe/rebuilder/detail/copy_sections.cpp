#include "pe/rebuilder/rebuilder.hpp"

namespace pe::detail {
    namespace {
        win::section_header_t assemble_section_header(const section_t& section) {
            return static_cast<win::section_header_t>(section);
        }

        template <any_image_t Img>
        void copy_sections_(Img* image, std::vector<std::uint8_t>& data) {
            /// Casting our buffer as the raw image
            auto* out_img = detail::buffer_pointer<to_raw_img_t<Img>>(data);
            auto* nt_headers = out_img->get_nt_headers();
            auto* file_header = &nt_headers->file_header;
            auto* optional_header = &nt_headers->optional_header;

            /// Obtaining sections pointer within the nt headers
            auto* sections = out_img->get_nt_headers()->get_sections();

            /// Raligning sections
            image->realign_sections();

            /// Validating that there's enough space for our sections
            auto sections_start = memory::address{sections};
            auto header_end = memory::address{out_img}.offset(optional_header->size_headers);
            if (sections_start + (sizeof(win::section_header_t) * (image->sections.size() + 1)) > header_end) {
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

                /// Copying section data
                auto sec_ptr = memory::address{data.data()}.offset(section.ptr_raw_data);
                if (auto write_res = sec_ptr.write(section.raw_data.data(), section.raw_data.size()); !write_res.has_value()) {
                    throw std::runtime_error("pe: rebuilder: unable to write section raw data");
                }

                /// Updating the data directories
                section.export_contained_dir(optional_header);
            }

            /// Updating the sections count
            file_header->num_sections = static_cast<std::uint16_t>(image->sections.size());
        }
    } // namespace

    void copy_sections(const ImgWrapped image, std::vector<std::uint8_t>& data) {
        return UNWRAP_IMAGE(void, copy_sections_);
    }
} // namespace pe::detail