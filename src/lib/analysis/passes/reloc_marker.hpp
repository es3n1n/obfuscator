#pragma once
#include "analysis/analysis.hpp"
#include "util/structs.hpp"

namespace analysis::passes {
    template <pe::any_image_t Img>
    struct reloc_marker_t {
        DEFAULT_CT_CTOR_DTOR(reloc_marker_t);
        NON_COPYABLE(reloc_marker_t);

        static bool apply_insn(Function<Img>* function [[maybe_unused]], insn_t& instruction, Img* image) {
            // Would be set to true if instruction contains imm/ip operands
            //
            const zasm::Imm* imm = instruction.find_operand_if<zasm::Imm>();
            const zasm::Mem* mem = instruction.find_operand_if<zasm::Mem>();

            // If there are no imm and ip references, then there shouldn't be any relocated data :thinking:
            //
            if (imm == nullptr && mem == nullptr) {
                return false;
            }

            /// Obtain needed stuff from pe
            const auto image_base = image->raw_image->get_nt_headers()->optional_header.image_base;
            const auto ptr_size = image->get_ptr_size();

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

            // At this point, we are 100% sure that imm is set to something, so we can ignore the `imm != 0` check.
            // If there's an imm with the size of uintptr_t, we should check maybe it's present in the .reloc dir
            //
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
