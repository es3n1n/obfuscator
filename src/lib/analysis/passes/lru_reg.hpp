#pragma once
#include "analysis/analysis.hpp"
#include "util/structs.hpp"

namespace analysis::passes {
    struct lru_reg_t {
        DEFAULT_CT_CTOR_DTOR(lru_reg_t);
        NON_COPYABLE(lru_reg_t);

        static bool apply_insn(const PassContext& ctx, const insn_t& instruction) {
            /// Collect all the registers and push them to LRU.
            /// \todo @es3n1n: We should track life time of registers
            for (auto& reg : easm::get_all_registers(*instruction.ref)) {
                ctx.function->lru_reg.push_known(reg.getId());
            }

            return true;
        }
    };
} // namespace analysis::passes
