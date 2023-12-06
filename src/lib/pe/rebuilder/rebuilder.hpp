#pragma once
#include "pe/rebuilder/detail/common.hpp"

namespace pe {
    namespace detail {
        void update_relocations(ImgWrapped, std::vector<std::uint8_t>& data);
        void init_header(ImgWrapped image, std::vector<std::uint8_t>& data);
        void copy_sections(ImgWrapped image, std::vector<std::uint8_t>& data);
        void update_checksum(ImgWrapped image, std::vector<std::uint8_t>& data);
    } // namespace detail

    template <any_image_t Img>
    struct rebuilder_ctx_t {
        Img* image;

        [[nodiscard]] detail::ImgWrapped wrap() {
            return detail::wrap_image(image);
        }
    };

    template <any_image_t Img>
    [[nodiscard]] std::vector<std::uint8_t> rebuild_pe(rebuilder_ctx_t<Img> ctx) {
        // Init result data
        //
        std::vector<std::uint8_t> result = {};

        // Updating .reloc section
        //
        detail::update_relocations(ctx.wrap(), result);

        // Reserving and copying the original header first
        //
        detail::init_header(ctx.wrap(), result);

        // Copying sections
        //
        detail::copy_sections(ctx.wrap(), result);

        // Update checksum
        //
        detail::update_checksum(ctx.wrap(), result);

        // We are done here
        //
        return result;
    }
} // namespace pe
