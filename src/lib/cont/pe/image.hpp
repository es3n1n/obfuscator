//
// Created by es3n1n on 2025-08-06.
//

#pragma once
#include <cont/base.hpp>
#include <linuxpe>

namespace cont::pe {
    class Image final : public ImageBase {
    public:
        explicit Image(const memory::address raw_image): ImageBase(raw_image) {
            initialize();
        }
        [[nodiscard]] ImageMode mode() const override;
        [[nodiscard]] bool verify_integrity() const override;

        [[nodiscard]] std::size_t get_image_base() const override;
        [[nodiscard]] std::size_t get_section_alignment() const override;
        [[nodiscard]] std::size_t get_file_alignment() const override;
        [[nodiscard]] std::size_t get_base_of_code() const override;

        [[nodiscard]] std::vector<std::uint8_t> rebuild_image() override;
        void realign_sections() override;

        void update_sections() override;
        void update_relocations() override;

        [[nodiscard]] win::data_directory_t* get_directory(win::directory_id dir_id);

    private:
        [[nodiscard]] win::image_x64_t* x64() const {
            return raw_image_.as<win::image_x64_t*>();
        }

        [[nodiscard]] win::image_x86_t* x86() const {
            return raw_image_.as<win::image_x86_t*>();
        }
    };

    template <typename Ty> concept AnyRawImage = traits::is_any_of_v<Ty, win::image_x64_t, win::image_x86_t>;
} // namespace cont::pe
