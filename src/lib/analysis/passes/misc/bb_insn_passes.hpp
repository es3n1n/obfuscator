#pragma once
#include "analysis/analysis.hpp"
#include "util/structs.hpp"

//
// This pass executes other standalone transforms that doesn't need to iter bb/insns by themselves,
// by using this pass we're reducing the number of iterations that we need to do, thus we reduce
// the time that we would need to spend in order to finish analysis.
//

namespace analysis::passes {
    template <pe::any_image_t Img>
    struct bb_insn_passes_t {
        DEFAULT_CTOR_DTOR(bb_insn_passes_t);
        NON_COPYABLE(bb_insn_passes_t);

        static bool apply(Function<Img>* function, Img* image);
    };
} // namespace analysis::passes
