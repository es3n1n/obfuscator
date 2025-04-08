#pragma once
#include "pe/pe.hpp"

#include <optional>
#include <stdexcept>
#include <zasm/zasm.hpp>

namespace easm {
    constexpr size_t kMaxEntryInstructionSize = 5; // jump in our case

    template <pe::any_image_t Img>
    constexpr zasm::x86::Gp sp_for_arch() {
        if constexpr (pe::is_x64_v<Img>) {
            return zasm::x86::rsp;
        } else {
            return zasm::x86::esp;
        }
    }

    constexpr zasm::BitSize sp_size_for_arch(const zasm::MachineMode machine_mode) {
        switch (machine_mode) {
        case zasm::MachineMode::AMD64:
            return zasm::toBitSize(sizeof(std::uint64_t) * CHAR_BIT);
        case zasm::MachineMode::I386:
            return zasm::toBitSize(sizeof(std::uint32_t) * CHAR_BIT);
        default:
            assert(false); // unknown arch
            std::unreachable();
        }
    }

    template <pe::any_image_t Img>
    constexpr zasm::BitSize sp_size_for_arch() {
        if constexpr (pe::is_x64_v<Img>) {
            return sp_size_for_arch(zasm::MachineMode::AMD64);
        } else {
            return sp_size_for_arch(zasm::MachineMode::I386);
        }
    }

    template <pe::any_image_t Img, typename... TArgs>
    constexpr zasm::Mem ptr(TArgs... args) {
        if constexpr (pe::is_x64_v<Img>) {
            return zasm::x86::qword_ptr(std::forward<TArgs>(args)...);
        } else {
            return zasm::x86::dword_ptr(std::forward<TArgs>(args)...);
        }
    }

    inline bool is_jcc_or_jmp(const zasm::Instruction& insn) {
        const auto mnemonic = insn.getMnemonic();

        return mnemonic >= ZYDIS_MNEMONIC_JB && mnemonic <= ZYDIS_MNEMONIC_JZ;
    }

    inline bool is_jcc_or_jmp(const zasm::InstructionDetail& insn) {
        const auto insn_info = insn.getInstruction();
        return is_jcc_or_jmp(insn_info);
    }

    inline bool is_ret(const zasm::Instruction& insn) {
        const auto mnemonic = insn.getMnemonic();
        return mnemonic.value() == ZYDIS_MNEMONIC_RET;
    }

    inline bool is_ret(const zasm::InstructionDetail& insn) {
        const auto insn_info = insn.getInstruction();
        return is_ret(insn_info);
    }

    inline bool affects_ip(const zasm::Instruction& insn) {
        if (is_jcc_or_jmp(insn)) {
            return true;
        }

        if (insn.getMnemonic().value() == ZYDIS_MNEMONIC_CALL) {
            return true;
        }

        bool result = false;
        for (std::size_t i = 0; i < insn.getOperandCount() && !result; ++i) {
            if (const auto* op_reg = insn.getOperandIf<zasm::Reg>(i); op_reg != nullptr) {
                result = op_reg->isIP();
                continue;
            }

            if (const auto* op_mem = insn.getOperandIf<zasm::Mem>(i); op_mem != nullptr) {
                result = op_mem->getBase().isIP();
            }
        }

        return result;
    }

    inline bool affects_ip(const zasm::InstructionDetail& insn) {
        const auto insn_info = insn.getInstruction();
        return affects_ip(insn_info);
    }

    struct jcc_t {
        bool conditional = false;
        std::optional<std::uint64_t> branch = std::nullopt;
        std::optional<const zasm::Label*> branch_label = std::nullopt;
    };

    inline jcc_t follow_jcc_or_jmp(const zasm::Instruction& insn) {
        if (!is_jcc_or_jmp(insn)) {
            throw std::runtime_error("Tried to follow non-jcc instruction");
        }

        jcc_t result = {};
        result.conditional = insn.getMnemonic().value() != ZYDIS_MNEMONIC_JMP;

        for (std::size_t i = 0; i < insn.getOperandCount(); ++i) {
            if (const auto* operand = insn.getOperandIf<zasm::Imm>(i)) {
                result.branch = std::make_optional<std::uint64_t>(operand->value<std::uint64_t>());
                break;
            }

            if (const auto* label = insn.getOperandIf<zasm::Label>(i)) {
                result.branch_label = std::make_optional(label);
                break;
            }
        }

        return result;
    }

    inline jcc_t follow_jcc_or_jmp(const zasm::InstructionDetail& insn) {
        const auto insn_info = insn.getInstruction();
        return follow_jcc_or_jmp(insn_info);
    }

    inline zasm::BitSize get_operand_size(const zasm::MachineMode machine_mode, const zasm::Operand& operand) {
        if (const auto* op_reg = operand.getIf<zasm::Reg>()) {
            return op_reg->getBitSize(machine_mode);
        }

        if (const auto* op_mem = operand.getIf<zasm::Mem>()) {
            return op_mem->getBitSize(machine_mode);
        }

        assert(false); // what's this? zasm::Imm is handled in the different function
        std::unreachable();
    }

    inline zasm::BitSize get_operand_size(const zasm::MachineMode machine_mode, const zasm::Instruction* insn, const std::size_t index) {
        const auto& operand = insn->getOperand(index);

        if (const auto* op_imm = operand.getIf<zasm::Imm>()) {
            /// Let's see if we know the size of operand
            const auto attributes = insn->getAttribs();

            // NOLINTBEGIN(readability-implicit-bool-conversion)
            if (attributes & zasm::x86::Attribs::OperandSize8) {
                return zasm::toBitSize(8);
            }

            if (attributes & zasm::x86::Attribs::OperandSize16) {
                return zasm::toBitSize(16);
            }

            if (attributes & zasm::x86::Attribs::OperandSize32) {
                return zasm::toBitSize(32);
            }

            if (attributes & zasm::x86::Attribs::OperandSize64) {
                return zasm::toBitSize(64);
            }
            // NOLINTEND(readability-implicit-bool-conversion)

            /// Push instruction always pushes the stack width bit size
            if (insn->getMnemonic().value() == ZYDIS_MNEMONIC_PUSH) {
                return sp_size_for_arch(machine_mode);
            }

            return op_imm->getBitSize();
        }

        return get_operand_size(machine_mode, operand);
    }

    inline zasm::x86::Gp to_gp(const zasm::Reg reg) {
        return zasm::x86::Gp{reg.getId()};
    }

    inline zasm::x86::Gp to_root_gp(const zasm::MachineMode machine_mode, const zasm::Reg reg) {
        return zasm::x86::Gp{reg.getRoot(machine_mode).getId()};
    }

    inline std::pair<zasm::x86::Gp, zasm::x86::Gp> to_gp_root_gp(const zasm::MachineMode machine_mode, const zasm::Reg reg) {
        return std::make_pair(to_gp(reg), to_root_gp(machine_mode, reg));
    }

    inline void assert_operand_used_reg(const zasm::MachineMode machine_mode [[maybe_unused]], const zasm::Instruction* insn, const std::size_t index,
                                        const zasm::Reg reg [[maybe_unused]]) {
        if (const auto* op_reg = insn->getOperandIf<zasm::Reg>(index)) {
            assert(to_root_gp(machine_mode, *op_reg).getId() != reg.getId());
        }

        if (const auto* op_mem = insn->getOperandIf<zasm::Mem>(index); op_mem != nullptr && op_mem->getBase().isValid()) {
            assert(to_root_gp(machine_mode, op_mem->getBase()).getId() != reg.getId());
        }
    }

    inline void assert_operand_size(const zasm::MachineMode machine_mode [[maybe_unused]], const zasm::Instruction* insn [[maybe_unused]],
                                    const std::size_t index [[maybe_unused]], const zasm::Reg reg [[maybe_unused]]) {
        assert(get_operand_size(machine_mode, insn, index) == reg.getBitSize(machine_mode));
    }

    inline bool is_sp(const zasm::MachineMode machine_mode, const zasm::Reg reg) {
        const auto sp_reg = static_cast<zasm::Reg::Id>(machine_mode == zasm::MachineMode::AMD64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP);
        return reg.getRoot(machine_mode).getId() == sp_reg;
    }

    inline bool affects_sp(const zasm::MachineMode machine_mode, const zasm::Instruction& insn) {
        for (std::size_t i = 0; i < insn.getOperandCount(); ++i) {
            if (const auto* op_mem = insn.getOperandIf<zasm::Mem>(i); op_mem != nullptr) {
                if (is_sp(machine_mode, op_mem->getBase())) {
                    return true;
                }
                continue;
            }

            if (const auto* op_reg = insn.getOperandIf<zasm::Reg>(i); op_reg != nullptr) {
                if (is_sp(machine_mode, *op_reg)) {
                    return true;
                }
            }
        }

        return false;
    }

    inline std::vector<zasm::Reg> get_all_registers(const zasm::Instruction& insn) {
        std::vector<zasm::Reg> result = {};

        for (std::size_t i = 0; i < insn.getOperandCount(); ++i) {
            if (const auto* op_mem = insn.getOperandIf<zasm::Mem>(i); op_mem != nullptr && op_mem->getBase().isValid()) {
                result.emplace_back(op_mem->getBase());
                continue;
            }

            if (const auto* op_reg = insn.getOperandIf<zasm::Reg>(i); op_reg != nullptr) {
                result.emplace_back(*op_reg);
            }
        }

        return result;
    }
} // namespace easm
