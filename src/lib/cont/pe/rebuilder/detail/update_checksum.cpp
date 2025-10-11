#include "cont/pe/rebuilder/rebuilder.hpp"

namespace cont::pe::detail {
    namespace {
        template <AnyRawImage Img>
        void update_checksum_(std::vector<std::uint8_t>& data) {
            /// Get the headers
            auto* out_img = reinterpret_cast<Img*>(data.data());

            /// Update checksum
            out_img->update_checksum(data.size());
        }
    } // namespace

    void update_checksum(const Image* image, std::vector<std::uint8_t>& data) {
        switch (image->mode()) {
        case ImageMode::X64: {
            update_checksum_<win::image_x64_t>(data);
            break;
        }
        case ImageMode::X86: {
            update_checksum_<win::image_x86_t>(data);
            break;
        }
        default:
            throw std::out_of_range("cont::pe::detail::update_checksum: Unsupported image mode");
        }
    }
} // namespace cont::pe::detail
