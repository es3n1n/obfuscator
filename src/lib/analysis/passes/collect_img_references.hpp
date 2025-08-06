#pragma once
#include "analysis/analysis.hpp"
#include "util/structs.hpp"

namespace analysis::passes {
    struct collect_img_references_t {
        DEFAULT_CT_CTOR_DTOR(collect_img_references_t);
        NON_COPYABLE(collect_img_references_t);

        static bool apply_insn(const PassContext& ctx, insn_t& instruction) {
            // This is a weird pass, since it checks by image base we can't get it to work with nameless functions
            if (!ctx.image.has_value()) {
                return false;
            }

            // Looking for IMMs in the insn
            //
            const auto* imm = instruction.find_operand_if<zasm::Imm>();
            if (imm == nullptr) {
                return false;
            }

            // Obtaining IMM value and image base
            //
            auto image = *ctx.image;
            const auto imm_value = imm->value<std::uint64_t>();
            const auto base_address = image->get_image_base();

            // Skip instruction if imm isn't in the range of image
            //
            if (imm_value < base_address) {
                return false;
            }

            // Remembering reference
            //
            ctx.function->image_references[imm_value - base_address].emplace_back(&instruction);
            return true;
        }
    };
} // namespace analysis::passes
