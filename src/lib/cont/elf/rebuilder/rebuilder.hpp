#pragma once
#include "cont/elf/image.hpp"
#include <es3n1n/common/progress.hpp>

namespace cont::elf {
    namespace detail {
        void init_header(Image* image, std::vector<std::uint8_t>& data);
        void copy_sections(Image* image, std::vector<std::uint8_t>& data);
        void write_relocations(Image* image, std::vector<std::uint8_t>& data);
        void build_section_headers(Image* image, std::vector<std::uint8_t>& data);
    } // namespace detail

    struct RebuilderContext {
        Image* image;
    };

    [[nodiscard]] inline std::vector<std::uint8_t> rebuild_elf(RebuilderContext& ctx) {
        // Init result data
        //
        std::vector<std::uint8_t> result = {};
        auto progress = progress::Progress("elf: rebuilding", 4);

        // Reserving and copying the original header first
        //
        detail::init_header(ctx.image, result);
        progress.step();

        // Copying segments
        //
        detail::copy_sections(ctx.image, result);
        progress.step();

        // Write relocations back
        //
        detail::write_relocations(ctx.image, result);
        progress.step();

        // Rebuild sht
        //
        detail::build_section_headers(ctx.image, result);
        progress.step();

        // We are done here
        //
        return result;
    }
} // namespace cont::pe
