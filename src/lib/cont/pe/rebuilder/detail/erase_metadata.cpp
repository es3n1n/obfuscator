#include "cont/pe/image.hpp"
#include "cont/pe/rebuilder/rebuilder.hpp"

namespace cont::pe::detail {
    namespace {
        template <AnyRawImage Img>
        void erase_metadata_(std::vector<std::uint8_t>& data) {
            auto* out_img = reinterpret_cast<Img*>(data.data());
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
            if (auto& dbg_dir = optional_header->data_directories.debug_directory; dbg_dir.size > 0) {
                std::memset(out_img->rva_to_ptr(dbg_dir.rva), 0, dbg_dir.size);
                dbg_dir.rva = 0;
                dbg_dir.size = 0;
            }

            /// Wipe checksum
            std::memset(&optional_header->checksum, 0, sizeof(optional_header->checksum));
        }
    } // namespace

    void erase_metadata(const Image* image, std::vector<std::uint8_t>& data) {
        switch (image->mode()) {
        case ImageMode::X64: {
            erase_metadata_<win::image_x64_t>(data);
            break;
        }
        case ImageMode::X86: {
            erase_metadata_<win::image_x86_t>(data);
            break;
        }
        default:
            throw std::out_of_range("cont::pe::detail::erase_metadata: Unsupported image mode");
        }
    }
} // namespace cont::pe::detail
