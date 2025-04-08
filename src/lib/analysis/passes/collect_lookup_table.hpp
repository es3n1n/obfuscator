#pragma once
#include "analysis/analysis.hpp"
#include "util/structs.hpp"

namespace analysis::passes {
    template <pe::any_image_t Img>
    struct collect_lookup_table_t {
        DEFAULT_CT_CTOR_DTOR(collect_lookup_table_t);
        NON_COPYABLE(collect_lookup_table_t);

        static bool apply_insn(Function<Img>* function, insn_t& instruction, Img* /*image*/) {
            if (!instruction.rva.has_value()) {
                /// \fixme @es3n1n: this could be pretty bad that we don't push newly
                /// created insns to the lookup table, although we should be just fine without them
                return true;
            }

            // Building rva<->insn lookup table
            //
            function->instructions_lookup[*instruction.rva] = &instruction;
            return true;
        }
    };
} // namespace analysis::passes
