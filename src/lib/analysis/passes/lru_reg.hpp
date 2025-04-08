#pragma once
#include "analysis/analysis.hpp"
#include "util/structs.hpp"

namespace analysis::passes {
    template <pe::any_image_t Img>
    struct lru_reg_t {
        DEFAULT_CT_CTOR_DTOR(lru_reg_t);
        NON_COPYABLE(lru_reg_t);

        static bool apply_insn(Function<Img>* function, const insn_t& instruction, Img* /*image*/) {
            /// Collect all the registers and push them to LRU
            for (auto& reg : easm::get_all_registers(*instruction.ref)) {
                /// \note @es3n1n: We are pushing only known registers to avoid xmm/ymm/zmm stuff, we only need GP32/64
                function->lru_reg.push_known(reg.getId());
            }

            return true;
        }
    };
} // namespace analysis::passes
