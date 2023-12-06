#include "analysis/analysis.hpp"
#include "util/passes.hpp"

#include "analysis/passes/label_references.hpp"
#include "analysis/passes/misc/bb_insn_passes.hpp"

namespace analysis {
    template <pe::any_image_t Img>
    void Function<Img>::apply_passes(Img* image) {
        // @note: @es3n1n: for the apply_bb/apply_insn callbacks please check out the file
        // `analysis/transforms/misc/bb_insn_passes.hpp`, pass that would need to iter bb/insns
        // by themselves should be inserted here
        //
        ::passes::apply< //
            passes::bb_insn_passes_t<Img>, //
            passes::label_references_t<Img> //
            >(this, image);
    }

    template <pe::any_image_t Img>
    void Function<Img>::calc_range() {
        // Reset state
        //
        range.start = ULLONG_MAX;
        range.end = nullptr;

        // Iterating over instructions and updating range
        //
        bb_storage->iter_insns([this](const insn_t& instruction) -> void {
            if (!instruction.rva.has_value()) {
                return;
            }

            if (instruction.rva < range.start) {
                range.start = *instruction.rva;
            }

            if (instruction.rva > range.end) {
                range.end = *instruction.rva;
            }
        });
    }

    PE_DECL_TEMPLATE_CLASSES(Function);
} // namespace analysis
