#include "mathop/operations/impl/util.hpp"
#include "util/random.hpp"

namespace mathop::operations {
    /// \brief Emulate the math operation under the two operands
    /// \param op1 lhs
    /// \return emulated result
    ArgumentImm Not::emulate(ArgumentImm op1, std::optional<ArgumentImm>) const {
        ArgumentImm result;
        std::visit(
            [&]<typename Ty>(Ty&& op1_value) -> void { //
                result.emplace<std::decay_t<Ty>>(~op1_value);
            },
            op1);
        return result;
    }

    /// \brief Lift the revert operation for this math operation
    /// \param assembler zasm assembler
    /// \param operand dst operand
    void Not::lift_revert(zasm::x86::Assembler* assembler, const zasm::x86::Gp operand, std::optional<Argument>) const {
        assembler->not_(operand);
    }
} // namespace mathop::operations