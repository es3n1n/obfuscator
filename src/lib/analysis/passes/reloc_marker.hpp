#pragma once
#include "analysis/analysis.hpp"
#include "analysis/common/pass_context.hpp"
#include "util/structs.hpp"

namespace analysis::passes {
    struct reloc_marker_t {
        DEFAULT_CT_CTOR_DTOR(reloc_marker_t);
        NON_COPYABLE(reloc_marker_t);

        static bool apply_insn(PassContext& ctx, insn_t& instruction) {
            // Would be set to true if instruction contains imm/ip operands
            //
            const zasm::Imm* imm = instruction.find_operand_if<zasm::Imm>();
            const zasm::Mem* mem = instruction.find_operand_if<zasm::Mem>();

            // If there are no imm and ip references, then there shouldn't be any relocated data :thinking:
            //
            if (imm == nullptr && mem == nullptr) {
                return false;
            }

            /// \fixme @es3n1n: we always assume base at 0x0 for nameless functions
            std::uintptr_t image_base = 0;
            if (ctx.image.has_value()) {
                image_base = (*ctx.image)->get_image_base();
            }
            /// \fixme @es3n1n: move to util
            const auto ptr_size = ctx.image_mode == cont::ImageMode::X64 ? sizeof(std::uint64_t) : sizeof(std::uint32_t);

            // Force reloc mem
            if (mem != nullptr && mem->getBase().isIP()) {
                instruction.reloc = {
                    .imm_rva = memory::address{static_cast<uintptr_t>(mem->getDisplacement()) - image_base},
                    .type = insn_reloc_t::e_type::IP,
                    .offset = std::make_optional<std::uint8_t>(mem->getSegment().getOffset()),
                };

                return true;
            }

            // New instructions does not have any relocations
            //
            if (!instruction.rva.has_value() || !instruction.length.has_value()) {
                return true;
            }

            // For nameless functions there is no image,
            //  we should omit header checks as we don't have any header-relocations to proceed with.
            if (!ctx.image.has_value()) {
                return false;
            }
            auto* image = *ctx.image;

            // At this point, we are 100% sure that imm is set to something.
            // If there's an imm with the size of uintptr_t, we should check maybe it's present in the .reloc dir
            if (imm != nullptr && getBitSize(imm->getBitSize()) == (ptr_size * CHAR_BIT) && instruction.length >= ptr_size) {
                // Trying to find relocation from PE header within the instruction
                // \todo @es3n1n: check segments instead of just bruteforcing
                //
                for (std::size_t offset = 0; offset <= (*instruction.length - ptr_size); ++offset) {
                    auto iter = image->relocations.find(*instruction.rva + offset);
                    if (iter == image->relocations.end()) {
                        continue;
                    }

                    // Uh ohh we just found a relocation
                    //
                    instruction.reloc = {
                        .imm_rva = memory::address{static_cast<uintptr_t>(imm->value<std::int64_t>() - image_base)},
                        .type = insn_reloc_t::e_type::HEADER,
                        .offset = std::make_optional<std::uint8_t>(static_cast<std::uint8_t>(offset)),
                    };

                    // Erase the stored reloc info
                    //
                    image->relocations.erase(iter);

                    return true;
                }
            }

            // No reloc :sob:
            //
            return false;
        }
    };
} // namespace analysis::passes
