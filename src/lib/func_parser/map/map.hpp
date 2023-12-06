#pragma once
#include "func_parser/common/common.hpp"
#include "pe/common/types.hpp"
#include "util/files.hpp"

namespace func_parser::map {
    function_list_t discover_functions(const std::filesystem::path& map_path, const std::vector<pe::section_t>& sections);
} // namespace func_parser::map
