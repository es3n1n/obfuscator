#pragma once
#include "cont/base.hpp"
#include "func_parser/common/common.hpp"
#include <es3n1n/common/files.hpp>

namespace func_parser::map {
    function_list_t discover_functions(const std::filesystem::path& map_path, const std::vector<cont::Section>& sections);
} // namespace func_parser::map
