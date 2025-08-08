#pragma once
#include "analysis/common/debug.hpp"

namespace easm {
    inline void dump_program(const zasm::Program& program, const bool errors_only = true) {
        /// Iterating over the program nodes
        ///
        for (auto* node = program.getHead(); node != nullptr; node = node->getNext()) {
            // Handling `zasm::Data`
            //
            if (const auto* node_data = node->getIf<zasm::Data>(); node_data != nullptr) {
                logger::info("Data: {:#x}", node_data->valueAsU64());
                continue;
            }

            //
            // Handling `zasm::Instruction`
            //
            if (auto* node_insn = node->getIf<zasm::Instruction>(); node_insn != nullptr) {
                bool is_error = false;
                if (const auto& insn_info = node_insn->getDetail(program.getMode()); !insn_info) {
                    is_error = true;
                    logger::error<1>("unable to get instn info -- this is broken");
                }

                if (!is_error && errors_only) {
                    continue;
                }

                logger::info("Instruction: {}", ZydisMnemonicGetString(static_cast<ZydisMnemonic>(node_insn->getMnemonic().value())));

                if (const auto ops_count = node_insn->getOperandCount(); ops_count > 0) {
                    logger::info<1>("Operands:");

                    for (std::size_t i = 0; i < ops_count; ++i) {
                        const auto operand = node_insn->getOperand(i);

                        // NOLINTNEXTLINE(google-build-using-namespace)
                        using namespace analysis::debug::detail;
                        const auto operand_name_pair = operand_type_lookup.find(operand.getTypeIndex());
                        const auto operand_name = //
                            operand_name_pair == std::end(operand_type_lookup) ? unknown_type_name : operand_name_pair->second;

                        logger::info<2>("{}", operand_name);

                        if (const auto* p_imm = node_insn->getOperandIf<zasm::Imm>(i); p_imm) {
                            logger::info<3>("Value: {:#x}", p_imm->value<std::uint64_t>());
                        }

                        if (const auto* p_mem = node_insn->getOperandIf<zasm::Mem>(i); p_mem) {
                            const auto reg = static_cast<ZydisRegister_>(p_mem->getBase().getId());
                            logger::info<3>("expr: [{} + {:#x}]", reg == 0 ? "none" : ZydisRegisterGetString(reg), p_mem->getDisplacement());
                        }

                        if (const auto* p_reg = node_insn->getOperandIf<zasm::Reg>(i); p_reg) {
                            logger::info<3>("name: {}", ZydisRegisterGetString(static_cast<ZydisRegister>(p_reg->getId())));
                        }
                    }
                }

                continue;
            }

            //
            // Handling `zasm::EmbeddedLabel`
            //
            if (const auto* embedded_label = node->getIf<zasm::EmbeddedLabel>(); embedded_label != nullptr) {
                logger::info("Label");
            }
        }
    }
} // namespace easm