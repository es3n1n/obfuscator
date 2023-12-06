#pragma once
#include "analysis/analysis.hpp"
#include "util/structs.hpp"

namespace analysis::passes {
    template <pe::any_image_t Img>
    struct collect_img_references_t {
        DEFAULT_CTOR_DTOR(collect_img_references_t);
        NON_COPYABLE(collect_img_references_t);

        static bool apply_insn(Function<Img>* function, insn_t& instruction, Img* image) {
            // Looking for IMMs in the insn
            //
            const auto* imm = instruction.find_operand_if<zasm::Imm>();
            if (imm == nullptr) {
                return false;
            }

            // Obtaining IMM value and image base
            //
            const auto imm_value = imm->value<std::uint64_t>();
            const auto base_address = image->raw_image->get_nt_headers()->optional_header.image_base;

            // Skip instruction if imm isn't in the range of image
            //
            if (imm_value < base_address) {
                return false;
            }

            // Remembering reference
            //
            function->image_references[imm_value - base_address].emplace_back(&instruction);
            return true;
        }
    };
} // namespace analysis::passes
