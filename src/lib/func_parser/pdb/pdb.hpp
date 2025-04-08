#pragma once
#include "func_parser/common/common.hpp"
#include "pe/pe.hpp"
#include <es3n1n/common/files.hpp>

// \todo: @es3n1n: validate that the pdb could be used for provided file
// \todo: @es3n1n: add custom pdb server support?

namespace func_parser::pdb {
    function_list_t discover_functions(const std::filesystem::path& pdb_path, std::uint64_t base_of_code = 0ULL);

    inline function_list_t discover_functions(const win::cv_pdb70_t* code_view, const std::uint64_t base_of_code = 0ULL) {
        // Return an empty set if there's no code view
        //
        if (code_view == nullptr) {
            return {};
        }

        return discover_functions(code_view->pdb_name, base_of_code);
    }
} // namespace func_parser::pdb
