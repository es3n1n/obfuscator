#pragma once
#include "pe/pe.hpp"

namespace pe::common {
    template <any_raw_image_t Img>
    [[nodiscard]] bool is_valid(const Img* image) {
        return image->dos_header.e_magic == win::DOS_HDR_MAGIC;
    }
} // namespace pe::common
