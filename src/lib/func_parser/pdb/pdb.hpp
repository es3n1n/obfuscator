#pragma once
#include "cont/base.hpp"
#include "func_parser/common/common.hpp"
#include <es3n1n/common/files.hpp>

// \todo: @es3n1n: validate that the pdb could be used for provided file
// \todo: @es3n1n: add custom pdb server support?

namespace func_parser::pdb {
    function_list_t discover_functions(const std::filesystem::path& pdb_path, std::uint64_t base_of_code = 0ULL);
} // namespace func_parser::pdb
