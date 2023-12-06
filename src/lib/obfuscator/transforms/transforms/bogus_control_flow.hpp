#pragma once
#include "mathop/mathop.hpp"
#include "obfuscator/transforms/scheduler.hpp"
#include "obfuscator/transforms/transforms/util/bcf.hpp"
#include "obfuscator/transforms/transforms/util/opaque_predicates.hpp"

namespace obfuscator::transforms {
    template <pe::any_image_t Img>
    class BogusControlFlow final : public FunctionTransform<Img> {
    public:
        enum Var {
            MODE = 0,
            EXPR_SIZE = 1,
        };
        enum Mode {
            OPAQUE_PREDICATES = 0,
            RANDOM_PREDICATES = 1,
        };

        /// \brief Optional callback that initializes config variables
        void init_config() override {
            auto& mode = this->new_var(Var::MODE, "mode", false, TransformConfig::Var::Type::PER_FUNCTION, 1);
            mode.short_description(std::format("opaque predicates (+tamper) - {} || random predicates - {}", static_cast<int>(Mode::OPAQUE_PREDICATES),
                                               static_cast<int>(Mode::RANDOM_PREDICATES)));

            auto& expr_size = this->new_var(Var::EXPR_SIZE, "expr_size", false, TransformConfig::Var::Type::PER_FUNCTION, 15);
            expr_size.short_description("used only if mode is set to RANDOM_PREDICATES");
        }

        /// \brief Transform function
        /// \param ctx Transform context
        /// \param function Routine that it should transform
        void run_on_function(TransformContext& ctx, Function<Img>* function) override {
            auto mode = static_cast<Mode>(this->template get_var_value<int>(Var::MODE));
            assert(mode == Mode::OPAQUE_PREDICATES || mode == Mode::RANDOM_PREDICATES);

            auto expr_size = this->template get_var_value<int>(Var::EXPR_SIZE);
            assert(expr_size > 0);

            /// Iterating over the basic blocks
            for (auto& bb : function->bb_storage->temp_copy()) {
                /// We aren't modifying BBs with no successors
                if (bb->successors.empty()) {
                    continue;
                }

                /// Check chance
                if (!rnd::chance(ctx.shared_config.chance())) {
                    continue;
                }

                /// Generating a BCF stub
                transform_util::generate_bogus_confrol_flow<Img>(
                    function, bb.get(),
                    [&](std::shared_ptr<analysis::bb_t> new_bb) -> void {
                        /// Tamper data if needed
                        switch (mode) {
                        case Mode::OPAQUE_PREDICATES:
                            tamper_instructions(function, new_bb);
                            break;
                        default:
                            break;
                        }
                    },
                    [&](zasm::x86::Assembler* assembler, zasm::Label successor_label, zasm::Label dead_branch_label,
                        analysis::VarAlloc<Img>* var_alloc) -> void {
                        /// Generate predicate
                        switch (mode) {
                        case Mode::OPAQUE_PREDICATES:
                            transform_util::generate_opaque_predicate(assembler, successor_label, dead_branch_label, var_alloc);
                            break;
                        case Mode::RANDOM_PREDICATES:
                            gen_random_predicate(assembler, successor_label, dead_branch_label, *var_alloc, expr_size);
                            break;
                        default:
                            assert(false);
                            break;
                        }
                    });
            }
        }

    private:
        static void tamper_instructions(Function<Img>* function, std::shared_ptr<analysis::bb_t> bb) {
            /// Iterating over the copied instructions
            for (const auto& insn : bb->instructions) {
                /// Skip instructions that affect IP
                if (easm::affects_ip(*insn->ref)) {
                    continue;
                }

                /// Iterate over the operands
                for (std::size_t i = 0; i < insn->ref->getOperandCount(); ++i) {
                    /// Tamper mem
                    if (auto* op_mem = insn->ref->template getOperandIf<zasm::Mem>(i)) {
                        if (auto base = op_mem->getBase(); base.isValid()) {
                            op_mem->setBase(function->lru_reg.get_for_bits(base.getBitSize(function->machine_mode), true));
                        }
                        if (op_mem->getDisplacement()) {
                            op_mem->setDisplacement(rnd::number<std::int8_t>());
                        }
                    }

                    /// Tamper reg
                    if (auto* op_reg = insn->ref->template getOperandIf<zasm::Reg>(i)) {
                        insn->ref->setOperand(i, function->lru_reg.get_for_bits(op_reg->getBitSize(function->machine_mode), true));
                    }

                    /// Tamper imm
                    if (auto* op_imm = insn->ref->template getOperandIf<zasm::Imm>(i)) {
                        op_imm->setValue(rnd::number<std::int8_t>(0, 4));
                    }
                }
            }
        }

        static void gen_random_predicate(zasm::x86::Assembler* as, zasm::Label successor_label, zasm::Label dead_branch_label,
                                         analysis::VarAlloc<Img>& var_alloc, const std::size_t expr_size) {
            /// Generate the expr, alloc x
            auto expr = mathop::ExpressionGenerator::get().generate(zasm::BitSize::_32, expr_size);
            auto lreg = var_alloc.get_gp32_lo(true);

            /// Push x, lift expr
            var_alloc.push(as);
            expr.lift_revert(as, lreg);

            /// Compare with random result, doesn't really matter
            as->cmp(lreg, zasm::Imm(rnd::number<std::int16_t>()));
            var_alloc.pop(as);

            /// Two semantically identical branches
            as->jz(dead_branch_label);
            as->jmp(successor_label);
        }
    };
} // namespace obfuscator::transforms
