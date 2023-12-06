#pragma once
#include "pe/pe.hpp"

namespace pe::arch {
    template <any_raw_image_t Img>
    [[nodiscard]] bool is_x64(const Img* image) {
        return image->get_nt_headers()->file_header.machine == win::machine_id::amd64;
    }

    template <any_raw_image_t Img>
    [[nodiscard]] zasm::MachineMode guess_machine_mode(const Img* image) {
        if (is_x64(image)) {
            return zasm::MachineMode::AMD64;
        }

        return zasm::MachineMode::I386;
    }
} // namespace pe::arch