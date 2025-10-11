#pragma once
#include "cont/pe/image.hpp"
#include <es3n1n/common/progress.hpp>

namespace cont::pe {
    namespace detail {
        void update_relocations(Image* /*image*/, const std::vector<std::uint8_t>& data);
        void init_header(Image* image, std::vector<std::uint8_t>& data);
        void copy_sections(Image* image, std::vector<std::uint8_t>& data);
        void update_checksum(const Image* image, std::vector<std::uint8_t>& data);
        void erase_metadata(const Image* image, std::vector<std::uint8_t>& data);
    } // namespace detail

    struct RebuilderContext {
        Image* image;
    };

    [[nodiscard]] inline std::vector<std::uint8_t> rebuild_pe(RebuilderContext& ctx) {
        // Init result data
        //
        std::vector<std::uint8_t> result = {};
        auto progress = progress::Progress("pe: rebuilding", 5);

        // Updating .reloc section
        //
        detail::update_relocations(ctx.image, result);
        progress.step();

        // Reserving and copying the original header first
        //
        detail::init_header(ctx.image, result);
        progress.step();

        // Copying sections
        //
        detail::copy_sections(ctx.image, result);
        progress.step();

        // Update checksum
        //
        detail::update_checksum(ctx.image, result);
        progress.step();

        /// Wipe metadata
        //
        detail::erase_metadata(ctx.image, result);
        progress.step();

        // We are done here
        //
        return result;
    }
} // namespace cont::pe
