#include "mathop/operations/impl/util.hpp"
#include <es3n1n/common/random.hpp>

namespace mathop::operations {
    /// \brief Emulate the math operation under the two operands
    /// \param op1 lhs
    /// \param op2 rhs
    /// \return emulated result
    ArgumentImm Dec::emulate(ArgumentImm op1, std::optional<ArgumentImm> op2 [[maybe_unused]]) const {
        ArgumentImm result;
        std::visit(
            [&]<typename Ty>(Ty op1_value) -> void { //
                result.emplace<std::decay_t<Ty>>(op1_value - 1);
            },
            op1);
        return result;
    }

    /// \brief Lift the revert operation for this math operation
    /// \param assembler zasm assembler
    /// \param operand dst operand
    /// \param argument optional rhs
    void Dec::lift_revert(zasm::x86::Assembler* assembler, const zasm::x86::Gp operand, std::optional<Argument> argument [[maybe_unused]]) const {
        assembler->inc(operand);
    }
} // namespace mathop::operations