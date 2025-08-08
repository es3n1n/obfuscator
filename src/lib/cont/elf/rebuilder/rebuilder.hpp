#pragma once
#include "cont/elf/image.hpp"
#include <es3n1n/common/progress.hpp>

namespace cont::elf {
    namespace detail {
        void update_relocations(Image* image);
        void init_header(Image* image, std::vector<std::uint8_t>& result);
        void copy_sections(Image* image, std::vector<std::uint8_t>& result);
    } // namespace detail

    struct RebuilderContext {
        Image* image;
    };

    [[nodiscard]] inline std::vector<std::uint8_t> rebuild_elf(const RebuilderContext& ctx) {
        // Init result data
        //
        std::vector<std::uint8_t> result = {};
        auto progress = progress::Progress("elf: rebuilding", 3);

        detail::update_relocations(ctx.image);
        progress.step();

        detail::init_header(ctx.image, result);
        progress.step();

        detail::copy_sections(ctx.image, result);
        progress.step();

        // We are done here
        //
        return result;
    }
} // namespace cont::elf
