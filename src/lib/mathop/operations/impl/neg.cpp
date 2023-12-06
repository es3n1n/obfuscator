#include "mathop/operations/impl/util.hpp"
#include "util/random.hpp"

namespace mathop::operations {
    /// \brief Emulate the math operation under the two operands
    /// \param op1 lhs
    /// \param op2 rhs
    /// \return emulated result
    ArgumentImm Neg::emulate(ArgumentImm op1, std::optional<ArgumentImm> op2) const {
        ArgumentImm result;
        std::visit(
            [&]<typename Ty>(Ty&& op1_value) -> void { //
                result.emplace<std::decay_t<Ty>>(-op1_value);
            },
            op1);
        return result;
    }

    /// \brief Lift the revert operation for this math operation
    /// \param assembler zasm assembler
    /// \param operand dst operand
    /// \param argument optional rhs
    void Neg::lift_revert(zasm::x86::Assembler* assembler, zasm::x86::Gp operand, std::optional<Argument> argument) const {
        assembler->neg(operand);
    }
} // namespace mathop::operations