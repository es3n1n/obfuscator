#include "pe/rebuilder/rebuilder.hpp"

namespace pe::detail {
    namespace {
        template <any_image_t Img>
        void erase_metadata_(Img* image, std::vector<std::uint8_t>& data) {
            auto* out_img = detail::buffer_pointer<to_raw_img_t<Img>>(data);
            auto* nt_headers = out_img->get_nt_headers();
            auto* file_header = &nt_headers->file_header;
            auto* optional_header = &nt_headers->optional_header;

            /// Wipe rich header
            constexpr auto rich_offset = 0x80;
            const auto rich_size = out_img->dos_header.e_lfanew - rich_offset;
            std::memset(data.data() + rich_offset, 0, rich_size);

            /// Wipe linker version
            std::memset(&optional_header->linker_version, 0, sizeof(optional_header->linker_version));

            /// Alter number of symbols and timestamp
            file_header->num_symbols = std::numeric_limits<int>::max();
            std::memset(&file_header->timedate_stamp, 0, sizeof(file_header->timedate_stamp));

            /// Wipe debug directory
            /// \todo: wipe pdb path
            std::memset(&optional_header->data_directories.debug_directory, 0, optional_header->data_directories.debug_directory.size);

            /// Wipe checksum
            std::memset(&optional_header->checksum, 0, sizeof(optional_header->checksum));

            /// Wipe section names
            auto* sections = nt_headers->get_sections();
            for (std::size_t i = 0; i < std::max(static_cast<std::size_t>(file_header->num_sections), image->sections.size()); ++i) {
                std::memset(&sections[i].name, 0, sizeof(sections[i].name));
            }
        }
    } // namespace

    void erase_metadata(const ImgWrapped image, std::vector<std::uint8_t>& data) {
        UNWRAP_IMAGE(void, erase_metadata_);
    }
} // namespace pe::detail
