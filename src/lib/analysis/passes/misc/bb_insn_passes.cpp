#include "analysis/passes/misc/bb_insn_passes.hpp"

#include "analysis/common/common.hpp"
#include "analysis/passes/collect_img_references.hpp"
#include "analysis/passes/collect_lookup_table.hpp"
#include "analysis/passes/lru_reg.hpp"
#include "analysis/passes/reloc_marker.hpp"

namespace analysis::passes {
    namespace {
        template <pe::any_image_t Img>
        bool on_insn(Function<Img>* function, insn_t& instruction, Img* image) {
            bool result = false;

            result |= reloc_marker_t<Img>::apply_insn(function, instruction, image);
            result |= collect_img_references_t<Img>::apply_insn(function, instruction, image);
            result |= collect_lookup_table_t<Img>::apply_insn(function, instruction, image);
            result |= lru_reg_t<Img>::apply_insn(function, instruction, image);

            return result;
        }
    } // namespace

    template <pe::any_image_t Img>
    bool bb_insn_passes_t<Img>::apply(Function<Img>* function, Img* image) {
        bool result = false;

        // Iterating over BB and invoking callbacks
        //
        //
        function->bb_storage->iter_bbs([&](bb_t& basic_block) -> void {
            // Iterating over instructions and invoking callbacks
            //
            std::for_each(basic_block.instructions.begin(), basic_block.instructions.end(), [&function, &image, &result](auto& instruction) -> void { //
                result |= on_insn<Img>(function, *instruction, image);
            });
        });

        return result;
    }

    PE_DECL_TEMPLATE_STRUCTS(bb_insn_passes_t);
} // namespace analysis::passes
