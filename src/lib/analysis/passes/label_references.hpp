#pragma once
#include "analysis/analysis.hpp"
#include "util/format.hpp"
#include "util/structs.hpp"

namespace analysis::passes {
    template <pe::any_image_t Img>
    struct label_references_t {
        DEFAULT_CT_CTOR_DTOR(label_references_t);
        NON_COPYABLE(label_references_t);

        static bool apply(Function<Img>* function, Img* /*image*/) {
            // Iterating over referenced RVAs within the function
            //
            for (const auto& [referenced_insn_rva, insn_ptrs] : function->image_references) {
                // Skip if reference is not within the function
                //
                if (!(referenced_insn_rva >= function->range.start && referenced_insn_rva <= function->range.end)) {
                    continue;
                }

                // Skip empty lists, huh?
                //
                if (insn_ptrs.empty()) [[unlikely]] {
                    continue;
                }

                // Formatting referenced location
                // \fixme: @es3n1n: add a template param to address class and add conversions to it
                const auto referenced_loc_name = format::loc(static_cast<std::int32_t>(referenced_insn_rva.inner()));

                // Creating label for the referenced loc
                //
                const auto referenced_loc_label = function->program.get()->createLabel(referenced_loc_name.c_str());
                const auto referenced_loc_label_node = function->program.get()->bindLabel(referenced_loc_label);
                if (!referenced_loc_label_node) [[unlikely]] {
                    throw std::runtime_error("analysis: Unable to bind the label");
                }

                // Moving label node to the referenced instruction
                //
                const auto referenced_insn = function->instructions_lookup.find(referenced_insn_rva);
                if (referenced_insn == function->instructions_lookup.end()) [[unlikely]] {
                    throw std::runtime_error("analysis: Unable to find referenced insn");
                }
                function->program.get()->moveBefore(referenced_insn->second->node_ref, *referenced_loc_label_node);
                referenced_insn->second->bb_ref->push_label(*referenced_loc_label_node, function->bb_provider.get());

                // Iterating over instructions that referenced this RVA
                //
                for (auto* referrer_ptr : insn_ptrs) {
                    // Looking for the imm in this instruction
                    //
                    const auto imm_operand_index = referrer_ptr->template find_operand_index_if<zasm::Imm>();

                    // No imm, huh?
                    //
                    if (!imm_operand_index.has_value()) [[unlikely]] {
                        throw std::runtime_error("analysis: got an invalid imm operand index");
                    }

                    // Swapping imm with the label
                    //
                    referrer_ptr->ref->setOperand(imm_operand_index.value(), referenced_loc_label);
                }
            }

            return true;
        }
    };
} // namespace analysis::passes
