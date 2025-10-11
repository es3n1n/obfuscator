#include <algorithm>

#include "analysis/passes/misc/bb_insn_passes.hpp"

#include "analysis/common/common.hpp"
#include "analysis/passes/collect_img_references.hpp"
#include "analysis/passes/collect_lookup_table.hpp"
#include "analysis/passes/lru_reg.hpp"
#include "analysis/passes/reloc_marker.hpp"

namespace analysis::passes {
    namespace {
        bool on_insn(PassContext& ctx, insn_t& instruction) {
            bool result = false;

            result |= reloc_marker_t::apply_insn(ctx, instruction);
            result |= collect_img_references_t::apply_insn(ctx, instruction);
            result |= collect_lookup_table_t::apply_insn(ctx, instruction);
            result |= lru_reg_t::apply_insn(ctx, instruction);

            return result;
        }
    } // namespace

    bool bb_insn_passes_t::apply(PassContext& pass_context) {
        bool result = false;

        // Iterating over BB and invoking callbacks
        //
        //
        pass_context.function->bb_storage->iter_bbs([&](bb_t& basic_block) -> void {
            // Iterating over instructions and invoking callbacks
            //
            std::ranges::for_each(basic_block.instructions, [&pass_context, &result](auto& instruction) -> void { //
                result |= on_insn(pass_context, *instruction);
            });
        });

        return result;
    }
} // namespace analysis::passes
