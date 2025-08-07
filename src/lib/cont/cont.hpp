#pragma once
#include "elf/image.hpp"
#include "pe/image.hpp"

namespace cont {
    [[nodiscard]] inline ContImageType get_image_type(std::span<uint8_t> image_data) {
        if (image_data.size() >= sizeof(win::DOS_HDR_MAGIC) &&
            memory::address(image_data.data()).read<std::remove_cv_t<decltype(win::DOS_HDR_MAGIC)>>() == win::DOS_HDR_MAGIC) {
            return ContImageType::PE;
        }
        if (image_data.size() >= SELFMAG && std::memcmp(image_data.data(), ELFMAG, SELFMAG) == 0) {
            return ContImageType::ELF;
        }
        throw std::runtime_error("cont: get_image_type: Unsupported image type");
    }
} // namespace cont
