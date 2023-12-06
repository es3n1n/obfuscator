#pragma once
#include "func_parser/common/common.hpp"
#include "pe/pe.hpp"

namespace func_parser::sanitizer {
    template <pe::any_image_t Img>
    function_list_t sanitize_function_list(function_list_t items, const Img* image) noexcept {
        // Obtaining executable sections
        //
        const auto exec_sections = image->find_sections_if([](const pe::section_t& sec) -> bool { //
            return sec.characteristics.mem_execute;
        });

        // Iterating over functions
        //
        std::erase_if(items, [exec_sections](const function_t& item) -> bool {
            // Erasing invalid functions
            //
            if (!item.valid) {
                logger::debug("func_parser: sanitizing: !valid: {}", item);
                return true;
            }

            // Checking whether function is in an executable section or not
            //
            bool in_exec_mem = false;
            for (auto& sec : exec_sections) {
                in_exec_mem = item.rva >= sec.virtual_address && item.rva <= (sec.virtual_address + sec.virtual_size);

                if (in_exec_mem) {
                    break;
                }
            }

            if (!in_exec_mem) {
                logger::debug("func_parser: sanitizing: !in_exec_mem: {}", item);
            }

            // If a function isn't in an executable memory, then we should get rid of it
            // because its obviously not a function, and we can't do much with it
            //
            return !in_exec_mem;
        });

        return items;
    }
} // namespace func_parser::sanitizer
