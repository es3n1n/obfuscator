#include "pe/rebuilder/rebuilder.hpp"

namespace pe::detail {
    namespace {
        template <any_image_t Img>
        void init_header_(Img* image, std::vector<std::uint8_t>& data) {
            // Obtaining header structs
            //
            auto* nt_headers = image->raw_image->get_nt_headers();
            auto* optional_header = &nt_headers->optional_header;

            // Obtaining some other stuff from the header
            //
            auto& last_section = image->find_last_section();

            // Estimating image sizes
            //
            const auto raw_image_size = last_section.ptr_raw_data + last_section.size_raw_data;
            const auto virtual_image_size = last_section.virtual_address + last_section.virtual_size;

            // Updating values in the header
            // \todo: @es3n1n: size_code, size_init_data, size_uninit_data, base_of_code, num_rva_sizes
            // \note: @es3n1n: the sections count field is updated within the `copy_sections` pass!
            //
            optional_header->size_image = virtual_image_size;

            // Reserving header size
            //
            data.resize(raw_image_size);

            // Copying the original header
            // NOLINTNEXTLINE
            std::memcpy(data.data(), image->raw_image, optional_header->size_headers);
        }
    } // namespace

    void init_header(const ImgWrapped image, std::vector<std::uint8_t>& data) {
        UNWRAP_IMAGE(void, init_header_);
    }
} // namespace pe::detail
