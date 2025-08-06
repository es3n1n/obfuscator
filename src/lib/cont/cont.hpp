#pragma once
#include "pe/image.hpp"

namespace cont {
    enum struct ContImageType : std::uint8_t {
        PE = 0,
        ELF,
    };

    [[nodiscard]] inline ContImageType get_image_type(std::span<uint8_t> image_data) {
        if (image_data.size() >= sizeof(win::DOS_HDR_MAGIC) &&
            memory::address(image_data.data()).read<std::remove_cv_t<decltype(win::DOS_HDR_MAGIC)>>() == win::DOS_HDR_MAGIC) {
            return ContImageType::PE;
        }
        throw std::runtime_error("cont: get_image_type: Unsupported image type");
    }
} // namespace cont
