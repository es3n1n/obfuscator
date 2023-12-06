#pragma once
#include <algorithm>

#include "func_parser/common/common.hpp"

namespace func_parser::combiner {
    inline function_list_t combine_function_lists(std::vector<function_list_t>& lists) {
        // No lists?
        //
        if (lists.empty()) {
            return {};
        }

        // Pick first entry as base entry
        //
        function_list_t result = lists.at(0);

        // Nothing to combine
        //
        if (lists.size() == 1) {
            return result;
        }

        // Iterating over other lists
        //
        std::for_each(lists.begin() + 1, lists.end(), [&result](const function_list_t& functions) -> void {
            std::ranges::for_each(functions, [&result](const function_t& function) -> void {
                // Looking for this function in result
                //
                const auto result_it = std::ranges::find_if(result, [function](const function_t& func) -> bool { //
                    return func.rva == function.rva;
                });

                // If this is something new that we haven't seen already then we
                // should just store it
                //
                if (result_it == result.end()) {
                    result.emplace_back(function);
                    return;
                }

                // Trying to merge attributes of a function
                //
                result_it->merge(function);
            });
        });

        // We are done here
        //
        return result;
    }
} // namespace func_parser::combiner
