#pragma once
#include "cont/base.hpp"

namespace sections {
    enum class e_section_t : std::uint8_t {
        RELOC = 0,
        CODE,
    };

    namespace detail {
        constexpr std::array RELOC_NAME = {'.', 's', '_', 'r', 'e', 'l', '\x00', '\x00'};
        constexpr std::array CODE_NAME = {'.', 's', '_', 'c', 'o', 'd', 'e', '\x00'};

        static_assert(RELOC_NAME.size() == LEN_SHORT_STR);
        static_assert(CODE_NAME.size() == LEN_SHORT_STR);

        constexpr cont::Section::Characteristics RELOC_CHARACTERISTICS = {.flags = 0x00210002}; // read, discard, contains init data
        constexpr cont::Section::Characteristics CODE_CHARACTERISTICS = {.flags = 0x00300001}; // contains code, exec, read
    } // namespace detail

    inline cont::Section get(const e_section_t sec) {
        switch (sec) {
        case e_section_t::RELOC: {
            return cont::Section{
                .name = detail::RELOC_NAME.data(),
                .virtual_size = 0U,
                .virtual_address = 0U,
                .size_raw_data = 0U,
                .ptr_raw_data = 0U,
                .characteristics = detail::RELOC_CHARACTERISTICS,
            };
        }
        case e_section_t::CODE: {
            return cont::Section{
                .name = detail::CODE_NAME.data(),
                .virtual_size = 0U,
                .virtual_address = 0U,
                .size_raw_data = 0U,
                .ptr_raw_data = 0U,
                .characteristics = detail::CODE_CHARACTERISTICS,
            };
        }
        default:
            throw std::out_of_range("sections::get: Unsupported section type");
        }
    }
} // namespace sections
