#pragma once

#include "util/memory/address.hpp"
#include "util/sections.hpp"
#include "util/structs.hpp"
#include "util/types.hpp"

#include "pe/common/types.hpp"

#include <functional>
#include <linuxpe>
#include <unordered_map>
#include <vector>
#include <zasm/base/mode.hpp>

// NOLINTBEGIN(bugprone-macro-parentheses)
#define PE_DECL_TEMPLATE_CLASSES(class_name)                \
    template class class_name<pe::Image<win::image_x64_t>>; \
    template class class_name<pe::Image<win::image_x86_t>>
#define PE_DECL_TEMPLATE_STRUCTS(struct_name)                 \
    template struct struct_name<pe::Image<win::image_x64_t>>; \
    template struct struct_name<pe::Image<win::image_x86_t>>
// NOLINTEND(bugprone-macro-parentheses)

namespace pe {
    /// Concept for raw images from linux-pe, could also probably check for `win::image_t`
    template <typename Ty> concept any_raw_image_t = types::is_any_of_v<Ty, win::image_x86_t, win::image_x64_t>;

    /// Wrapper around pe header data, could be improved and hopefully everything could be merged from the
    ///
    template <any_raw_image_t Img>
    class Image {
    public:
        explicit Image(Img* image): raw_image(image) {
            update_sections();
            update_relocations();
        }
        DEFAULT_CTOR_DTOR(Image);
        DEFAULT_COPY(Image);

        [[nodiscard]] bool is_x64() const;

        [[nodiscard]] bool is_valid() const;

        [[nodiscard]] std::vector<section_t> find_sections_if(const std::function<bool(const section_t&)>& pred) const;

        [[nodiscard]] win::cv_pdb70_t* find_codeview70() const;

        [[nodiscard]] zasm::MachineMode guess_machine_mode() const;

        [[nodiscard]] section_t& find_last_section() const;

        section_t& new_section(sections::e_section_t section, std::size_t size);
        section_t& new_section(std::string_view name, std::size_t size, //
                               win::section_characteristics_t characteristics);

        void realign_sections();

        [[nodiscard]] section_t* rva_to_section(std::uint32_t rva) const;

        template <typename Ty = std::uint8_t>
        [[nodiscard]] Ty* rva_to_ptr(const memory::address rva) const {
            const auto* section = rva_to_section(rva.as<std::uint32_t>());
            if (section == nullptr) {
                return nullptr;
            }

            const auto offset = rva.inner() - section->virtual_address;
            return memory::address{section->raw_data.data()}.offset(offset).template as<std::add_pointer_t<Ty>>();
        }

        template <typename Ty = std::size_t>
        [[nodiscard]] Ty get_ptr_size() const {
            return sizeof(std::conditional_t<std::is_same_v<Img, win::image_x64_t>, uint64_t, uint32_t>);
        }

        [[nodiscard]] win::data_directory_t* get_directory(win::directory_id dir_id) const {
            auto nt_hdrs = raw_image->get_nt_headers();
            if (nt_hdrs->optional_header.num_data_directories <= dir_id) {
                return nullptr;
            }

            return &nt_hdrs->optional_header.data_directories.entries[dir_id];
        }

        [[nodiscard]] std::vector<std::uint8_t> rebuild_pe_image();

    private:
        void update_sections();
        void update_relocations();

    public:
        /// A raw image instance, that contains all PE info
        Img* raw_image = nullptr;

        /// An unordered map that consists of {rva: reloc_info}
        std::unordered_map<memory::address, relocation_t> relocations = {};

        /// A sections list
        mutable std::vector<section_t> sections = {};
    };

    /// A concept for our Image, so that we can just use it within the templates
    template <typename Ty> concept any_image_t = types::is_any_of_v<Ty, Image<win::image_x86_t>, Image<win::image_x64_t>>;

    /// Converter from `Image` to `win::image_x**_t`
    template <typename Ty>
    struct to_raw_img {
        using type = std::conditional_t<std::is_same_v<Ty, Image<win::image_x64_t>>, win::image_x64_t, win::image_x86_t>;
    };

    /// Alias to the converter
    template <typename Ty>
    using to_raw_img_t = typename to_raw_img<Ty>::type;

    /// Image aliases
    using X64Image = Image<win::image_x64_t>;
    using X86Image = Image<win::image_x86_t>;
    using DefaultImage = X64Image;

    /// Check if image is x64
    template <any_image_t Img>
    struct is_x64_img {
        constexpr static bool value = std::is_same_v<Img, X64Image>;
    };

    /// Check if image arch is x86
    template <any_image_t Img>
    struct is_x86_img {
        constexpr static bool value = std::is_same_v<Img, X86Image>;
    };

    /// Alias for is_x64<Img>::value
    template <any_image_t Img>
    constexpr static bool is_x64_v = is_x64_img<Img>::value;

    /// Alias for is_x86<Img>::value
    template <any_image_t Img>
    constexpr static bool is_x86_v = is_x86_img<Img>::value;
} // namespace pe
