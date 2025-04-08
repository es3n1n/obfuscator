#pragma once
#include "analysis/analysis.hpp"
#include "analysis/var_alloc/var_alloc.hpp"

namespace obfuscator::transform_util {
    /// \brief Prevent decompilers from symbolic execution of our stubs, also trigger a bug in
    /// binja decompiler
    /// \tparam Img X64 or X86 image
    /// \param var_alloc Var allocator
    /// \param imm_bit_size Imm bit size
    /// \param program zasm Program
    /// \param assembler zasm Assembler
    /// \param first first node
    /// \param last last node
    template <pe::any_image_t Img>
    void anti_symbolic_execution(analysis::VarAlloc<Img>& var_alloc, const zasm::BitSize imm_bit_size, zasm::Program* program,
                                 zasm::x86::Assembler* assembler, zasm::Node* first, zasm::Node* last) {
        /// Set cursor at the beginning
        assembler->setCursor(first);

        /// Decl some allocated vars that we could use later
        /// Set to nullopt for now since we aren't sure if we'd need them yet
        std::optional<analysis::SymVar> xchg_enc_holder = std::nullopt;
        std::optional<analysis::SymVar> adc_const_holder = std::nullopt;
        std::optional<analysis::SymVar> cf_val_holder = std::nullopt;

        /// Initializer util
        auto init_vars = [&]() -> void {
            if (xchg_enc_holder.has_value()) {
                return;
            }

            /// Init vars
            xchg_enc_holder = var_alloc.get_for_bits(imm_bit_size);
            adc_const_holder = var_alloc.get_for_bits(imm_bit_size);
            cf_val_holder = var_alloc.get_for_bits(imm_bit_size);

            /// Load empty val for the CF value holder
            assembler->push(zasm::Imm(0));
            assembler->pop(cf_val_holder->root_gp());

            /// Save the cf value
            assembler->adc(cf_val_holder->root_gp(), cf_val_holder->root_gp());
        };

        /// Iterating over the nodes
        for (auto* cur = assembler->getCursor(); cur != nullptr && cur != last; cur = cur->getNext()) {
            /// Set the cursor, get the node
            auto* insn = cur->getIf<zasm::Instruction>();

            /// Not an instruction
            if (insn == nullptr) {
                continue;
            }

            /// Get the imm
            auto* imm = insn->getOperandIf<zasm::Imm>(1);
            if (imm == nullptr) {
                continue;
            }

            /// \fixme
            if (imm->getBitSize() == zasm::BitSize::_64) {
                continue;
            }

            /// For now let's modify only `xxx reg, 0x1337` instructions
            if (insn->getOperandIf<zasm::Reg>(0) == nullptr) {
                continue;
            }

            /// Set the cursor
            assembler->setCursor(cur->getPrev());

            /// Get some decryption keys
            const auto adc_key = rnd::number<std::int8_t>(0, 55);
            constexpr auto adc_cf_val = 1;

            /// Operand decryption with xchg (unsupported in ida)
            auto xchg_reencrypt_var = [&]<typename Ty>(Ty) -> void {
                /// Alloc some stack frame where the decrypted var would be stored at
                assembler->push(zasm::Imm(rnd::number<std::int16_t>()));
                /// Push encrypted constant
                assembler->mov(xchg_enc_holder->root_gp(), zasm::Imm(imm->value<Ty>() - (adc_key + adc_cf_val)));
                assembler->push(xchg_enc_holder->root_gp());
                /// Pop the encrypted val
                assembler->pop(xchg_enc_holder->root_gp());
                /// Push it on stack with xchg operation
                assembler->xchg(easm::ptr<Img>(easm::sp_for_arch<Img>()), xchg_enc_holder->root_gp());
                /// Load to register
                assembler->pop(xchg_enc_holder->root_gp());
            };

            /// Operand decryption with adc (unsupported in binja)
            auto adc_reencrypt_var = [&]<typename Ty>(Ty) -> void {
                /// Load the adc key
                assembler->push(zasm::Imm(adc_key));
                assembler->pop(adc_const_holder->root_gp());

                /// ADC
                assembler->stc(); // set CF to 1 so that it will add xchg_val + addc_val + 1 (binja doesn't track this)
                assembler->adc(*xchg_enc_holder, *adc_const_holder);
            };

            /// Invoke both
            auto reencrypt_var = [&]<typename Ty>(Ty) -> void {
                /// Init vars
                init_vars();
                /// Reencrypt stuff
                xchg_reencrypt_var(Ty{});
                adc_reencrypt_var(Ty{});
                /// Swap the operand
                insn->setOperand(1, *xchg_enc_holder);
            };

            /// Handle bit sizes (looks sketchy)
            /// \todo @es3n1n: some util that would do this
            switch (imm_bit_size) {
            case zasm::BitSize::_8:
                reencrypt_var(std::int8_t{});
                break;
            case zasm::BitSize::_16:
                reencrypt_var(std::int16_t{});
                break;
            case zasm::BitSize::_32:
                reencrypt_var(std::int32_t{});
                break;
            case zasm::BitSize::_64:
                reencrypt_var(std::int64_t{});
                break;
            default:
                break;
            }
            // break;
        }

        /// Hm
        if (!cf_val_holder.has_value()) [[unlikely]] {
            return;
        }

        /// Check the stored CF value
        const auto if_set = program->createLabel();
        assembler->test(cf_val_holder->root_gp(), cf_val_holder->root_gp());

        /// Bind the label and set cursor before the node
        assembler->bind(if_set);
        assembler->setCursor(assembler->getCursor()->getPrev());

        /// Clear CF if needed
        assembler->jnz(if_set);
        assembler->clc();
    }
} // namespace obfuscator::transform_util
