#pragma once
#include "obfuscator/transforms/scheduler.hpp"
#include <es3n1n/common/random.hpp>

namespace obfuscator::transforms {
    template <pe::any_image_t Img>
    class Substitution final : public BBTransform<Img> {
        /// Math operations replacement table
        using Cbk = std::function<void(Function<Img>*, analysis::insn_t*)>;
        std::unordered_map<std::uint16_t, std::vector<Cbk>> replacements;

        /// Temporary operand holder for substitutions
        struct tmp_op_holder_t {
            /// Constructor that captures the desired operands
            tmp_op_holder_t(zasm::x86::Assembler* assembler, analysis::VarAlloc<Img>& var_alloc, const analysis::insn_t* insn,
                            const zasm::Operand& operand, const std::optional<zasm::Operand>& other_operand = std::nullopt)
                : var_alloc(var_alloc), operand(operand), assembler(assembler) {
                /// No need to alloc anything
                if (operand.holds<zasm::Reg>()) {
                    return;
                }

                /// Get the bit size
                std::optional<zasm::BitSize> bitsize = std::nullopt;
                if (other_operand.has_value()) {
                    bitsize = easm::get_operand_size(insn->bb_ref->machine_mode, *other_operand);
                }
                bitsize = bitsize.value_or(operand.getBitSize(insn->bb_ref->machine_mode));
                assert(bitsize.has_value());

                /// Alloc sym var
                sym_var = this->var_alloc.get_for_bits(*bitsize, true);

                /// Push and setup sym var
                this->var_alloc.push(assembler);
                assembler->emit(zasm::Instruction(ZYDIS_MNEMONIC_MOV, 2, {sym_var->reg, operand}));

                /// Swap operand value
                this->operand = sym_var->reg;
            }

            /// Destructor that should pop sym vars if needed
            ~tmp_op_holder_t() {
                /// No need to pop anything
                if (!sym_var.has_value()) {
                    return;
                }

                /// Pop the vars from stack
                /// \note @es3n1n: We are reusing the same assembler pointer, hopefully its cursor would be
                /// set to where we should pop the vars :pray:
                var_alloc.pop(assembler);
            }

            /// Var alloc reference
            analysis::VarAlloc<Img>& var_alloc;
            /// Operand holder
            zasm::Operand operand;
            /// Stored symbolic var
            std::optional<analysis::SymVar> sym_var = std::nullopt;
            /// Assembler reference
            zasm::x86::Assembler* assembler;
        };

    public:
        /// \brief Optional callback that initializes config variables
        void init_config() override {
            /// x + y => x - (-y)
            replacements[ZYDIS_MNEMONIC_ADD].emplace_back([](Function<Img>* func, analysis::insn_t* insn) -> void {
                auto op1 = insn->ref->getOperand<zasm::Operand>(0);
                auto op2 = insn->ref->getOperand<zasm::Operand>(1);

                auto* as = *func->cursor->instead_of(insn->node_ref);
                auto var_alloc = func->var_alloc();
                auto op2_wrap = tmp_op_holder_t(as, var_alloc, insn, op2, op1);

                // x - (-y)
                as->emit(zasm::Instruction(ZYDIS_MNEMONIC_NEG, 1, {op2_wrap.operand}));
                as->emit(zasm::Instruction(ZYDIS_MNEMONIC_SUB, 2, {op1, op2_wrap.operand}));
            });

            /// x - y => x + (-y)
            replacements[ZYDIS_MNEMONIC_SUB].emplace_back([](Function<Img>* func, analysis::insn_t* insn) -> void {
                auto op1 = insn->ref->getOperand<zasm::Operand>(0);
                auto op2 = insn->ref->getOperand<zasm::Operand>(1);

                auto* as = *func->cursor->instead_of(insn->node_ref);
                auto var_alloc = func->var_alloc();
                auto op2_wrap = tmp_op_holder_t(as, var_alloc, insn, op2, op1);

                // x + (-y)
                as->emit(zasm::Instruction(ZYDIS_MNEMONIC_NEG, 1, {op2_wrap.operand}));
                as->emit(zasm::Instruction(ZYDIS_MNEMONIC_ADD, 2, {op1, op2_wrap.operand}));
            });

            /// x & y => (x ^ ~y) & x
            replacements[ZYDIS_MNEMONIC_AND].emplace_back([](Function<Img>* func, analysis::insn_t* insn) -> void {
                auto op1 = insn->ref->getOperand<zasm::Operand>(0);
                auto op2 = insn->ref->getOperand<zasm::Operand>(1);

                auto* as = *func->cursor->instead_of(insn->node_ref);
                auto var_alloc = func->var_alloc();
                auto op2_wrap = tmp_op_holder_t(as, var_alloc, insn, op2, op1);

                /// ~y
                as->emit(zasm::Instruction(ZYDIS_MNEMONIC_NOT, 1, {op2_wrap.operand}));

                /// ~y ^= x
                as->emit(zasm::Instruction(ZYDIS_MNEMONIC_XOR, 2, {op2_wrap.operand, op1}));

                /// & x
                as->emit(zasm::Instruction(ZYDIS_MNEMONIC_AND, 2, {op2_wrap.operand, op1}));

                /// mov to x
                as->emit(zasm::Instruction(ZYDIS_MNEMONIC_AND, 2, {op1, op2_wrap.operand}));
            });
        }

        /// \brief Transform zasm node
        /// \param function Routine that it should transform
        /// \param bb BB that it should transform
        void run_on_bb(TransformContext& /*ctx*/, Function<Img>* function, analysis::bb_t* bb) override {
            /// Iterating over the instructions
            for (auto& insn : bb->temp_insns_copy()) {
                if (easm::affects_sp(function->machine_mode, *insn->ref)) {
                    continue;
                }

                auto it = replacements.find(insn->ref->getMnemonic().value());
                if (it == std::end(replacements)) {
                    continue;
                }

                auto& cb = rnd::item(it->second);
                cb(function, insn.get());
            }
        }
    };
} // namespace obfuscator::transforms
