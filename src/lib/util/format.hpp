#pragma once
#include "util/sections.hpp"
#include <format>
#include <string>

namespace format {
    inline std::string loc(const std::int32_t rva) {
        return std::format("loc_{:x}", rva);
    }

    inline std::string sec(const sections::e_section_t sec) {
        return name(sec);
    }
} // namespace format
