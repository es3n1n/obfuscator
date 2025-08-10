#pragma once
#include "analysis/analysis.hpp"
#include "analysis/var_alloc/var_alloc.hpp"
#include "obfuscator/function.hpp"

namespace obfuscator::transform_util {
    /// \brief Generate a new copy of the BB successor and make a [opaque] predicate
    /// \tparam Img X64 or X86 Image
    /// \param function Obfuscator function ptr
    /// \param bb BB ptr
    /// \param post_generation_callback Post generation callback (to tamper instructions or something)
    /// \param predicate_generator Predicate generator
    inline void
    generate_bogus_control_flow(Function* function, analysis::bb_t* bb, const std::function<void(analysis::bb_t*)>& post_generation_callback,
                                // NOLINTNEXTLINE(performance-unnecessary-value-param)
                                std::function<void(zasm::x86::Assembler*, zasm::Label, zasm::Label, analysis::VarAlloc*)> predicate_generator) {

        /// Get the last non-jmp insn
        const auto last_insn = bb->last_non_jmp_insn(function->program.get(), true);
        if (!last_insn.has_value()) {
            return;
        }

        /// Get the successor
        const auto successor = (*last_insn)->linear_successor();
        if (successor->instructions.empty()) {
            return;
        }

        /// Place successor start label
        const auto successor_label = function->program->createLabel();
        auto* as = *function->cursor->before(successor->node_at(0));
        as->bind(successor_label);

        /// Crete the label that would be placed at the beginning of the "dead" branch
        const auto dummy_bb_label = function->program->createLabel();

        /// Get the var alloc
        auto var_alloc = function->var_alloc();

        /// Temporary disable oserver
        function->observer->stop();

        /// Setup dead branch
        as = *function->cursor->after((*last_insn)->node_ref);
        as->bind(dummy_bb_label);
        const auto label_node = as->getCursor();

        /// Create dead branch
        const auto new_bb = function->bb_storage->copy_bb(successor, as, function->program.get(), function->bb_provider.get());
        new_bb->push_label(label_node, function->bb_provider.get());

        /// Tamper instructions, if needed
        post_generation_callback(new_bb.get());

        /// Re-enable observer
        function->observer->start();

        /// Set the cursor, generate predicate
        as = *function->cursor->after((*last_insn)->node_ref);
        predicate_generator(as, successor_label, dummy_bb_label, &var_alloc);

        /// Update successors, predecessors
        /// \fixme @es3n1n: Temporary commented out, uncomment as soon as the fixme in observer is fixed
        // bb->successors.emplace_back(new_bb);
        // new_bb->predecessors.emplace_back(bb);
    }
} // namespace obfuscator::transform_util