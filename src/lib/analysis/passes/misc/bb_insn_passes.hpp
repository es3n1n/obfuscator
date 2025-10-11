#pragma once
#include "analysis/common/pass_context.hpp"
#include "util/structs.hpp"

//
// This pass executes other standalone transforms that doesn't need to iter bb/insns by themselves,
// by using this pass we're reducing the number of iterations that we need to do, thus we reduce
// the time that we would need to spend in order to finish analysis.
//

namespace analysis::passes {
    struct bb_insn_passes_t {
        DEFAULT_CT_CTOR_DTOR(bb_insn_passes_t);
        NON_COPYABLE(bb_insn_passes_t);

        static bool apply(PassContext& pass_context);
    };
} // namespace analysis::passes
