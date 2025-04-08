#include "mathop/operations/impl/util.hpp"
#include <es3n1n/common/random.hpp>

namespace mathop::operations {
    /// \brief Emulate the math operation under the two operands
    /// \param op1 lhs
    /// \param op2 rhs
    /// \return emulated result
    ArgumentImm Xor::emulate(ArgumentImm op1, std::optional<ArgumentImm> op2) const {
        ArgumentImm result;
        std::visit(
            [&]<typename Ty>(Ty op1_value) -> void { //
                // \todo @es3n1n: Add unsigned safe casts
                result.emplace<std::decay_t<Ty>>(op1_value ^ std::get<std::decay_t<Ty>>(*op2)); // NOLINT(hicpp-signed-bitwise)
            },
            op1);
        return result;
    }

    /// \brief Lift the revert operation for this math operation
    /// \param assembler zasm assembler
    /// \param operand dst operand
    /// \param argument optional rhs
    void Xor::lift_revert(zasm::x86::Assembler* assembler, zasm::x86::Gp operand, const std::optional<Argument> argument) const {
        lift(
            argument, detail::none,
            [assembler, operand](const zasm::x86::Gp reg) -> void { //
                assembler->xor_(operand, reg);
            },
            [assembler, operand](const zasm::Imm imm) -> void { //
                assembler->xor_(operand, imm);
            });
    }

    /// \brief Generate a random second operand
    /// \param lhs Operand 1
    /// \return Generated operand
    ArgumentImm Xor::generate_rhs(const ArgumentImm lhs) const {
        return detail::generate_random_argument_in_range(lhs);
    }
} // namespace mathop::operations