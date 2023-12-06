#pragma once
#include <../../../vendor/linux-pe/includes/linuxpe>
#include <cstdint>

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

        constexpr win::section_characteristics_t RELOC_CHARACTERISTICS = {.flags = 0x42000040}; // read, discard, init
        constexpr win::section_characteristics_t CODE_CHARACTERISTICS = {.flags = 0x60000020}; // contains code, exec, read
    } // namespace detail

    inline std::string name(const e_section_t sec) {
        switch (sec) {
        case e_section_t::RELOC:
            return detail::RELOC_NAME.data();
        case e_section_t::CODE:
            return detail::CODE_NAME.data();
        }

        std::unreachable();
    }

    inline win::section_characteristics_t characteristics(const e_section_t section) {
        switch (section) {
        case e_section_t::RELOC:
            return detail::RELOC_CHARACTERISTICS;
        case e_section_t::CODE:
            return detail::CODE_CHARACTERISTICS;
        }

        std::unreachable();
    }
} // namespace sections
