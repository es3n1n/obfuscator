#pragma once
#include "func_parser/common/common.hpp"

#include <LLVMDemangle.h>

namespace func_parser::demangler {
    inline function_list_t demangle_functions(function_list_t items) noexcept {
        // Iterating over the functions and demangling their names
        //
        std::ranges::for_each(items, [](function_t& item) -> void {
            if (auto* demangled = LLVMDemangle(item.name.c_str())) {
                item.name = demangled;
                std::free(demangled);
            }
        });

        return items;
    }
} // namespace func_parser::demangler
