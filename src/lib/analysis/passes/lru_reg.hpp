#pragma once
#include "analysis/analysis.hpp"
#include "util/structs.hpp"

namespace analysis::passes {
    template <pe::any_image_t Img>
    struct lru_reg_t {
        DEFAULT_CTOR_DTOR(lru_reg_t);
        NON_COPYABLE(lru_reg_t);

        static bool apply_insn(Function<Img>* function, const insn_t& instruction, Img*) {
            /// Collect all the registers and push them to LRU
            for (auto& reg : easm::get_all_registers(*instruction.ref)) {
                function->lru_reg.push(reg.getId());
            }

            return true;
        }
    };
} // namespace analysis::passes
