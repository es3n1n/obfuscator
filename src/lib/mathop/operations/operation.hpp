#pragma once
#include "util/structs.hpp"

#include <cstdint>
#include <optional>
#include <variant>
#include <stdexcept>

#include <zasm/zasm.hpp>

namespace mathop {
    /// Argument variants (probably should be moved to some other file)
    using ArgumentImm = std::variant<std::int64_t, std::int32_t, std::int16_t, std::int8_t>;
    using Argument = std::variant<zasm::Reg, std::int64_t, std::int32_t, std::int16_t, std::int8_t>;

    /// \brief Convert argumentimm to argument
    /// \param argument_imm Argument imm
    /// \return converted argument
    [[nodiscard]] inline Argument convert(const ArgumentImm argument_imm) {
        Argument result;
        std::visit([&]<typename Ty>(Ty&& value) -> void { result.emplace<std::decay_t<Ty>>(std::forward<Ty>(value)); }, argument_imm);
        return result;
    }

    /// \brief Generate an imm argument for the bitsize
    /// \param bit_size bit size
    /// \param value imm value
    /// \return argument imm
    [[nodiscard]] inline ArgumentImm imm_for_bits(const zasm::BitSize bit_size, const std::int64_t value = 0) {
        ArgumentImm result;
        switch (getBitSize(bit_size)) {
        case 8:
            result.emplace<std::int8_t>(static_cast<std::int8_t>(value));
            break;
        case 16:
            result.emplace<std::int16_t>(static_cast<std::int16_t>(value));
            break;
        case 32:
            result.emplace<std::int32_t>(static_cast<std::int32_t>(value));
            break;
        case 64:
            result.emplace<std::int64_t>(static_cast<std::int64_t>(value));
            break;
        default:
            throw std::runtime_error("imm_for_bits: unsupported bitsize");
        }
        return result;
    }

    /// \brief Convert ArgumentImm to zasm Imm
    /// \param value Imm argument value
    /// \return converted zasm::Imm
    [[nodiscard]] inline zasm::Imm imm_to_zasm(const ArgumentImm value) {
        return std::visit([&]<typename Ty>(Ty&& visited_val) -> zasm::Imm { return zasm::Imm(std::forward<Ty>(visited_val)); }, value);
    }

    /// \brief Convert constant to ArgumentImm with saving the type of lhs
    /// \param lhs lhs
    /// \param value value to convert
    /// \return converted ArgumentImm
    [[nodiscard]] inline ArgumentImm const_to_imm_for_rhs(const ArgumentImm lhs, const std::uint64_t value) {
        return std::visit([&]<typename Ty>(Ty&&) -> ArgumentImm { return ArgumentImm(static_cast<Ty>(value)); }, lhs);
    }

    /// \brief Math operation representation
    class Operation {
    public:
        DEFAULT_CTOR(Operation);
        virtual ~Operation() = default;

        /// \brief Indicates whether this operation should have a second argument or not
        virtual bool has_second_operand() const = 0;

        /// \brief Emulate the math operation under the two operands
        /// \param op1 lhs
        /// \param op2 rhs
        /// \return emulated result
        virtual ArgumentImm emulate(ArgumentImm op1, std::optional<ArgumentImm> op2 = std::nullopt) const = 0;

        /// \brief Lift the revert operation for this math operation
        /// \param assembler zasm assembler
        /// \param operand dst operand
        /// \param argument optional rhs
        virtual void lift_revert(zasm::x86::Assembler* assembler, zasm::x86::Gp operand, std::optional<Argument> argument = std::nullopt) const = 0;

        /// \brief Generate a random second operand
        /// \param lhs Operand 1
        /// \return Generated operand
        virtual ArgumentImm generate_rhs(ArgumentImm lhs) const {
            return const_to_imm_for_rhs(lhs, 0);
        }
    };

    /// \brief Math operation with one operand
    class OperationOneOperand : public Operation {
    public:
        /// \brief Indicates whether this operation should have a second argument or not
        bool has_second_operand() const override {
            return false;
        }
    };

    /// \brief Math operation with two operands
    class OperationTwoOperands : public Operation {
    public:
        /// \brief Indicates whether this operation should have a second argument or not
        bool has_second_operand() const override {
            return true;
        }
    };
} // namespace mathop
