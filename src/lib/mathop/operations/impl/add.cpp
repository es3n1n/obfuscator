#include "mathop/operations/impl/util.hpp"

namespace mathop::operations {
    /// \brief Emulate the math operation under the two operands
    /// \param op1 lhs
    /// \param op2 rhs
    /// \return emulated result
    ArgumentImm Add::emulate(ArgumentImm op1, std::optional<ArgumentImm> op2) const {
        ArgumentImm result;
        std::visit(
            [&]<typename Ty>(Ty&& op1_value) -> void { //
                using Decay = std::decay_t<Ty>;
                result.emplace<Decay>(op1_value + std::get<Decay>(*op2));
            },
            op1);
        return result;
    }

    /// \brief Lift the revert operation for this math operation
    /// \param assembler zasm assembler
    /// \param operand dst operand
    /// \param argument optional rhs
    void Add::lift_revert(zasm::x86::Assembler* assembler, zasm::x86::Gp operand, std::optional<Argument> argument) const {
        lift(
            argument, detail::none,
            [assembler, operand](const zasm::x86::Gp reg) -> void { //
                assembler->sub(operand, reg);
            },
            [assembler, operand](const zasm::Imm imm) -> void { //
                assembler->sub(operand, imm);
            });
    }

    /// \brief Generate a random second operand
    /// \param lhs Operand 1
    /// \return Generated operand
    ArgumentImm Add::generate_rhs(const ArgumentImm lhs) const {
        return detail::generate_random_argument_in_range(lhs);
    }
} // namespace mathop::operations