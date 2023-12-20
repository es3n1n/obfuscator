#pragma once
#include "mathop/operations/operation.hpp"

#define MATHOP_OPERATION_STUB(name, base_name)                                                                                     \
    class name final : public base_name {                                                                                          \
    public:                                                                                                                        \
        ArgumentImm emulate(ArgumentImm op1, std::optional<ArgumentImm> op2) const override;                                       \
        void lift_revert(zasm::x86::Assembler* assembler, zasm::x86::Gp operand, std::optional<Argument> argument) const override; \
        ArgumentImm generate_rhs(ArgumentImm lhs) const override;                                                                  \
    }

#define MATHOP_OPERATION_STUB_NO_RHS(name, base_name)                                                                              \
    class name final : public base_name {                                                                                          \
    public:                                                                                                                        \
        ArgumentImm emulate(ArgumentImm op1, std::optional<ArgumentImm> op2) const override;                                       \
        void lift_revert(zasm::x86::Assembler* assembler, zasm::x86::Gp operand, std::optional<Argument> argument) const override; \
    }

#define MATHOP_OPERATION_ONE_OP(name) MATHOP_OPERATION_STUB_NO_RHS(name, mathop::OperationOneOperand)
#define MATHOP_OPERATION_TWO_OPS(name) MATHOP_OPERATION_STUB(name, mathop::OperationTwoOperands)

namespace mathop::operations {
    /// + -
    MATHOP_OPERATION_TWO_OPS(Add);
    MATHOP_OPERATION_TWO_OPS(Sub);

    /// ^
    MATHOP_OPERATION_TWO_OPS(Xor);

    /// +1 -1
    MATHOP_OPERATION_ONE_OP(Inc);
    MATHOP_OPERATION_ONE_OP(Dec);

    /// -
    MATHOP_OPERATION_ONE_OP(Neg);

    /// ~
    MATHOP_OPERATION_ONE_OP(Not);

    /// \todo @es3n1n: rotl, rotr, nand, nor, and, or, mul, div, lshl, lshr
} // namespace mathop::operations

#undef MATHOP_OPERATION_TWO_OPS
#undef MATHOP_OPERATION_ONE_OP
#undef MATHOP_OPERATION_STUB_NO_RHS
#undef MATHOP_OPERATION_STUB
