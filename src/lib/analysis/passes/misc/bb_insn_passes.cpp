#include "analysis/passes/misc/bb_insn_passes.hpp"

#include "analysis/common/common.hpp"
#include "analysis/passes/collect_img_references.hpp"
#include "analysis/passes/collect_lookup_table.hpp"
#include "analysis/passes/lru_reg.hpp"
#include "analysis/passes/reloc_marker.hpp"

namespace analysis::passes {
    namespace {
        template <pe::any_image_t Img>
        bool on_insn(PassContext<Img>& ctx, insn_t& instruction) {
            bool result = false;

            result |= reloc_marker_t<Img>::apply_insn(ctx, instruction);
            result |= collect_img_references_t<Img>::apply_insn(ctx, instruction);
            result |= collect_lookup_table_t<Img>::apply_insn(ctx, instruction);
            result |= lru_reg_t<Img>::apply_insn(ctx, instruction);

            return result;
        }
    } // namespace

    template <pe::any_image_t Img>
    bool bb_insn_passes_t<Img>::apply(PassContext<Img>& pass_context) {
        bool result = false;

        // Iterating over BB and invoking callbacks
        //
        //
        pass_context.function->bb_storage->iter_bbs([&](bb_t& basic_block) -> void {
            // Iterating over instructions and invoking callbacks
            //
            std::for_each(basic_block.instructions.begin(), basic_block.instructions.end(), [&pass_context, &result](auto& instruction) -> void { //
                result |= on_insn<Img>(pass_context, *instruction);
            });
        });

        return result;
    }

    PE_DECL_TEMPLATE_STRUCTS(bb_insn_passes_t);
} // namespace analysis::passes
