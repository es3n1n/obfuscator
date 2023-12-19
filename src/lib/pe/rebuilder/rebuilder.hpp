#pragma once
#include "pe/rebuilder/detail/common.hpp"
#include "util/progress.hpp"

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
        auto progress = util::Progress("pe: rebuilding", 4);

        // Updating .reloc section
        //
        detail::update_relocations(ctx.wrap(), result);
        progress.step();

        // Reserving and copying the original header first
        //
        detail::init_header(ctx.wrap(), result);
        progress.step();

        // Copying sections
        //
        detail::copy_sections(ctx.wrap(), result);
        progress.step();

        // Update checksum
        //
        detail::update_checksum(ctx.wrap(), result);
        progress.step();

        // We are done here
        //
        return result;
    }
} // namespace pe
