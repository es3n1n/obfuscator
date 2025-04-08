#include "pe/rebuilder/rebuilder.hpp"

namespace pe::detail {
    namespace {
        template <any_image_t Img>
        void update_checksum_(Img* img [[maybe_unused]], std::vector<std::uint8_t>& data) {
            /// Get the headers
            auto* out_img = detail::buffer_pointer<to_raw_img_t<Img>>(data);

            /// Update checksum
            out_img->update_checksum(data.size());
        }
    } // namespace

    void update_checksum(const ImgWrapped image, std::vector<std::uint8_t>& data) {
        UNWRAP_IMAGE(void, update_checksum_);
    }
} // namespace pe::detail
