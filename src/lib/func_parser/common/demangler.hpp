#pragma once
#include "func_parser/common/common.hpp"

#include <algorithm>
#include <LLVMDemangle.h>

namespace func_parser::demangler {
    inline void demangle_functions(function_list_t& items) {
        // Iterating over the functions and demangling their names
        //
        std::ranges::for_each(items, [](function_t& item) -> void {
            if (auto* demangled = LLVMDemangle(item.name.c_str())) {
                item.name = demangled;
                // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc,cppcoreguidelines-owning-memory)
                std::free(demangled);
            }
        });
    }
} // namespace func_parser::demangler
