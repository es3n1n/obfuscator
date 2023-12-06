#pragma once
#include "mathop/operations/operations.hpp"
#include "util/random.hpp"
#include "util/types.hpp"

#include <algorithm>
#include <vector>

namespace mathop {
    /// \brief Operation and the second operand for it (if needed)
    class OperationValue {
    public:
        DEFAULT_DTOR(OperationValue);
        DEFAULT_COPY(OperationValue);
        explicit OperationValue(Operation* operation, const zasm::BitSize bit_size): operation_(operation), size_(bit_size) {
            /// We need to generate second operands only for the operations that needs them
            if (!operation_->has_second_operand()) {
                return;
            }

            /// Generate the expression and set it to both lift/emulation arguments
            emulation_rhs_ = operation_->generate_rhs(imm_for_bits(bit_size));
            lift_rhs_ = convert(emulation_rhs_.value());
        }

        /// \brief Emulate the operation
        /// \param lhs Lhs imm value
        /// \return Calculated value
        [[nodiscard]] ArgumentImm emulate(const ArgumentImm lhs) const {
            return operation_->emulate(lhs, emulation_rhs_);
        }

        /// \brief Lift revert operation to x86 code
        /// \param assembler zasm assembler
        /// \param dst_reg destination register
        void lift_revert(zasm::x86::Assembler* assembler, const zasm::x86::Gp dst_reg) const {
            return operation_->lift_revert(assembler, dst_reg, lift_rhs_);
        }

        /// \brief Get the emulation argument
        /// \return optional imm
        [[nodiscard]] std::optional<ArgumentImm> emulation_rhs() const {
            return emulation_rhs_;
        }

        /// \brief Set the emulation argument
        /// \param value New optional imm value
        void emulation_rhs(const std::optional<ArgumentImm>& value) {
            emulation_rhs_ = value;
        }

        /// \brief Get the lift argument
        /// \return optional argument (imm/reg)
        [[nodiscard]] std::optional<Argument> lift_rhs() const {
            return lift_rhs_;
        }

        /// \brief Set the lift argument
        /// \param value New optional argument value (imm/reg)
        void lift_rhs(const std::optional<Argument>& value) {
            lift_rhs_ = value;
        }

        /// \brief Returns true if this operation has a second operand
        /// \return bool
        [[nodiscard]] bool has_rhs() const {
            return emulation_rhs_.has_value() || lift_rhs_.has_value();
        }

        /// \brief Get the operands size
        /// \return operands size in bits
        [[nodiscard]] zasm::BitSize bit_size() const {
            return size_;
        }

    private:
        /// \brief Operation ptr
        Operation* operation_ = nullptr;
        /// \brief Arguments size
        zasm::BitSize size_ = {};
        /// \brief Emulation rhs (imm only)
        std::optional<ArgumentImm> emulation_rhs_ = std::nullopt;
        /// \brief Lift rhs (imm/register)
        std::optional<Argument> lift_rhs_ = std::nullopt;
    };

    /// \brief Expression with operations
    class Expression {
    public:
        DEFAULT_CTOR_DTOR(Expression);
        DEFAULT_COPY(Expression);

        /// \brief Emplace new operation
        /// \param operation mathop::Operation ptr
        /// \param bit_size operands bit size
        /// \return emplaced operation
        OperationValue& emplace_operation(Operation* operation, const zasm::BitSize bit_size) {
            return operations_.emplace_back(operation, bit_size);
        }

        /// \brief Emulate the whole expression
        /// \param start_value expression input
        /// \return calculated value
        [[nodiscard]] ArgumentImm emulate(const ArgumentImm start_value) const {
            ArgumentImm result = start_value;

            for (auto& operation : operations_) {
                result = operation.emulate(result);
            }

            return result;
        }

        /// \brief Lift reversion of this expression to x86 code
        /// \param assembler zasm assembler
        /// \param dst dst register
        void lift_revert(zasm::x86::Assembler* assembler, const zasm::x86::Gp dst) {
            /// Iterating over the reverted operations list
            for (auto it = operations_.rbegin(); it != operations_.rend(); std::advance(it, 1)) {
                it->lift_revert(assembler, dst);
            }
        }

        /// \brief Iterator begin
        /// \return operations begin
        auto begin() {
            return operations_.begin();
        }

        /// \brief Iterator const begin
        /// \return operations const begin
        auto begin() const {
            return operations_.begin();
        }

        /// \brief Iterator end
        /// \return operations end
        auto end() {
            return operations_.end();
        }

        /// \brief Iterator const end
        /// \return operations const end
        auto end() const {
            return operations_.end();
        }

    private:
        /// \brief A list of operations
        std::vector<OperationValue> operations_ = {};
    };

    /// \brief Expression generator
    class ExpressionGenerator : public types::Singleton<ExpressionGenerator> {
    public:
        DEFAULT_DTOR(ExpressionGenerator);
        NON_COPYABLE(ExpressionGenerator);

        /// \brief Operations initializer
        ExpressionGenerator() {
            operations_.emplace_back(std::make_unique<operations::Add>());
            operations_.emplace_back(std::make_unique<operations::Sub>());
            operations_.emplace_back(std::make_unique<operations::Inc>());
            operations_.emplace_back(std::make_unique<operations::Dec>());
            operations_.emplace_back(std::make_unique<operations::Xor>());
            operations_.emplace_back(std::make_unique<operations::Neg>());
            operations_.emplace_back(std::make_unique<operations::Not>());
        }

        /// \brief Generate a random math expression
        /// \param bit_size Operands bit size
        /// \param num_operations Number of operations
        /// \return Expression
        [[nodiscard]] Expression generate(const zasm::BitSize bit_size, const std::size_t num_operations) {
            Expression result;

            /// Generate the random expressions
            for (std::size_t i = 0; i < num_operations; ++i) {
                auto& operation = rnd::item(operations_);
                result.emplace_operation(operation.get(), bit_size);
            }

            return result;
        }

    private:
        /// \brief List of supported operations
        std::vector<std::unique_ptr<Operation>> operations_ = {};
    };
} // namespace mathop
