#pragma once
#include "mathop/mathop.hpp"
#include "obfuscator/transforms/scheduler.hpp"
#include "obfuscator/transforms/transforms/util/anti_decompilers.hpp"

namespace obfuscator::transforms {
    template <pe::any_image_t Img>
    class ConstantCrypt final : public BBTransform<Img> {
    public:
        enum Var {
            EXPR_SIZE = 0
        };

        /// \brief Optional callback that initializes config variables
        void init_config() override {
            this->new_var(Var::EXPR_SIZE, "expr_size", false, TransformConfig::Var::Type::PER_FUNCTION, 5);
        }

        void transform_insn(const TransformContext& ctx, Function<Img>* function, analysis::insn_t* insn) const {
            /// Ignore relocated stuff
            if (insn->reloc.type == analysis::insn_reloc_t::e_type::HEADER) {
                return;
            }

            /// Don't really feel like messing around with something that affects IP
            if (easm::affects_ip(*insn->ref)) {
                return;
            }

            /// Looking up for immediate operands
            auto imm_op_index = insn->find_operand_index_if<zasm::Imm>();
            if (!imm_op_index.has_value()) {
                return;
            }
            const auto* imm_op = insn->ref->getOperandIf<zasm::Imm>(imm_op_index.value());

            /// Get its value, bitsize
            const auto imm_value = imm_op->value<std::uint64_t>();
            const auto imm_bitsize = easm::get_operand_size(function->machine_mode, insn->ref, 0).value_or(imm_op->getBitSize());

            /// Export all registers and push them to the LRU blacklist
            for (auto reg : easm::get_all_registers(*insn->ref)) {
                function->lru_reg.blacklist(reg.getId());
            }
            [[maybe_unused]] auto cleaner = function->lru_reg.auto_cleaner();

            /// Alloc some variables
            auto var_alloc = function->var_alloc();
            auto var_1 = var_alloc.get_for_bits(imm_bitsize);

            /// Set cursor
            auto as_opt = function->cursor->before(insn->node_ref);
            if (!as_opt.has_value()) {
                return;
            }
            auto as = *as_opt;

            /// Detect sp-related things before we lift our stuff
            std::optional<zasm::Mem*> sp_mem = std::nullopt;
            if (easm::affects_sp(function->machine_mode, *insn->ref)) {
                sp_mem = std::make_optional(insn->find_operand_if<zasm::Mem>());

                /// Uh oh, we don't know how to process other stuff (fixme)
                if (*sp_mem == nullptr || //
                    !easm::is_sp(function->machine_mode, (*sp_mem)->getBase())) {
                    logger::warn("constant_crypt: not sure how to process SP at {:#x}", *insn->rva);
                    return;
                }
            }

            /// Check the chance
            if (!rnd::chance(ctx.shared_config.chance())) {
                return;
            }

            /// Remember nodes where we should push/pop stuff
            auto* push_at = as->getCursor();
            auto* pop_at = insn->node_ref;

            /// Generate decryption
            const auto expr_size = this->template get_var_value<int>(Var::EXPR_SIZE);
            assert(expr_size > 0);
            auto expression = mathop::ExpressionGenerator::get().generate(imm_bitsize, expr_size);
            auto evaluated = expression.emulate(mathop::imm_for_bits(imm_bitsize, imm_value));

            /// Setup dst register and lift decryption
            as->mov(var_1, mathop::imm_to_zasm(evaluated));

            /// Lift decryption
            auto decryption_start_at = as->getCursor();
            expression.lift_revert(as, var_1);

            /// Remember the last decryption node
            auto decryption_ends_at = as->getCursor();

            /// Prevent symbolic execution
            transform_util::anti_symbolic_execution(var_alloc, imm_bitsize, function->program.get(), as, decryption_start_at, decryption_ends_at);

            /// Assert stuff before shit would hit the fan
            if (*imm_op_index == 1) {
                easm::assert_operand_size(function->machine_mode, insn->ref, 0, var_1);
                easm::assert_operand_used_reg(function->machine_mode, insn->ref, 0, var_1);
            }

            /// Update stack offset, if needed, because we're gonna change its layout with our pushes
            if (sp_mem.has_value()) {
                (*sp_mem)->setDisplacement((*sp_mem)->getDisplacement() + var_alloc.stack_size());
                logger::debug("constant_crypt: updated sp displacement at {:#x}", *insn->rva);
            }

            /// Swap the operand
            insn->ref->setOperand(*imm_op_index, var_1);

            /// Allocate vars on stack
            as = *function->cursor->after(push_at);
            var_alloc.push(as);
            var_alloc.push_flags(as);

            /// Pop flags if needed
            as = *function->cursor->before(pop_at);
            var_alloc.pop_flags(as);

            /// Deallocate vars from stack
            as = *function->cursor->after(pop_at);
            var_alloc.pop(as);
        }

        /// \brief Transform analysis insn
        /// \param ctx Transform context
        /// \param function Routine that it should transform
        /// \param bb BB that it should transform
        void run_on_bb(TransformContext& ctx, Function<Img>* function, analysis::bb_t* bb) override {
            for (auto& insn : bb->temp_insns_copy()) {
                transform_insn(ctx, function, insn.get());
            }
        }
    };
} // namespace obfuscator::transforms
