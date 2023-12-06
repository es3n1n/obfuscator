#pragma once
#include "analysis/analysis.hpp"
#include "analysis/var_alloc/var_alloc.hpp"

namespace obfuscator::transform_util {
    /// \brief
    /// \param as Zasm assembler ptr
    /// \param successor_label Successor
    /// \param dead_branch_label Dead branch
    /// \param var_alloc_ptr Var allocator
    template <pe::any_image_t Img>
    void generate_opaque_predicate(zasm::x86::Assembler* as, zasm::Label successor_label, zasm::Label dead_branch_label,
                                   analysis::VarAlloc<Img>* var_alloc_ptr) {
        /// I don't feel like changing these stubs -- fixme
        auto& var_alloc = *var_alloc_ptr;

        /// Alloc X
        auto x = var_alloc.get_gp32_lo();
        var_alloc.push(as);

        /// Mov random var from stack to x
        as->mov(x, zasm::x86::dword_ptr(easm::sp_for_arch<Img>(), rnd::number<uint32_t>(0, 0x250)));

        /// \note @es3n1n: Generated using `scripts/opaque_predicates_expr_gen`
        switch (rnd::number<std::size_t>(0, 66)) {
        // ((x << 16) & 6) == 0
        case 0: {
            as->shl(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(6));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((x & 16) & 7) == 0
        case 1: {
            as->and_(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(7));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x + 2) << 4) & 6) == 0
        case 2: {
            as->add(x, zasm::Imm(2));
            as->shl(x, zasm::Imm(4));
            as->and_(x, zasm::Imm(6));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x + 6) & 3) & 12) == 0
        case 3: {
            as->add(x, zasm::Imm(6));
            as->and_(x, zasm::Imm(3));
            as->and_(x, zasm::Imm(12));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x << 9) << 1) & 4) == 0
        case 4: {
            as->shl(x, zasm::Imm(9));
            as->shl(x, zasm::Imm(1));
            as->and_(x, zasm::Imm(4));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x << 3) & 4) + 4) == 4
        case 5: {
            as->shl(x, zasm::Imm(3));
            as->and_(x, zasm::Imm(4));
            as->add(x, zasm::Imm(4));
            as->cmp(x, zasm::Imm(4));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x << 1) & 1) << 9) == 0
        case 6: {
            as->shl(x, zasm::Imm(1));
            as->and_(x, zasm::Imm(1));
            as->shl(x, zasm::Imm(9));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x << 16) & 6) & 6) == 0
        case 7: {
            as->shl(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(6));
            as->and_(x, zasm::Imm(6));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x << 2) & 2) ^ 2) == 2
        case 8: {
            as->shl(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(2));
            as->xor_(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(2));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x << 9) ^ 11) & 11) == 11
        case 9: {
            as->shl(x, zasm::Imm(9));
            as->xor_(x, zasm::Imm(11));
            as->and_(x, zasm::Imm(11));
            as->cmp(x, zasm::Imm(11));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x >> 10) << 16) & 6) == 0
        case 10: {
            as->shr(x, zasm::Imm(10));
            as->shl(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(6));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x >> 6) & 11) & 4) == 0
        case 11: {
            as->shr(x, zasm::Imm(6));
            as->and_(x, zasm::Imm(11));
            as->and_(x, zasm::Imm(4));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x & 1) + 16) & 2) == 0
        case 12: {
            as->and_(x, zasm::Imm(1));
            as->add(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x & 11) << 4) & 6) == 0
        case 13: {
            as->and_(x, zasm::Imm(11));
            as->shl(x, zasm::Imm(4));
            as->and_(x, zasm::Imm(6));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x & 4) & 3) + 16) == 16
        case 14: {
            as->and_(x, zasm::Imm(4));
            as->and_(x, zasm::Imm(3));
            as->add(x, zasm::Imm(16));
            as->cmp(x, zasm::Imm(16));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x & 4) & 2) << 12) == 0
        case 15: {
            as->and_(x, zasm::Imm(4));
            as->and_(x, zasm::Imm(2));
            as->shl(x, zasm::Imm(12));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x & 11) & 3) & 12) == 0
        case 16: {
            as->and_(x, zasm::Imm(11));
            as->and_(x, zasm::Imm(3));
            as->and_(x, zasm::Imm(12));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x & 2) & 1) ^ 2) == 2
        case 17: {
            as->and_(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(1));
            as->xor_(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(2));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x & 8) ^ 1) & 2) == 0
        case 18: {
            as->and_(x, zasm::Imm(8));
            as->xor_(x, zasm::Imm(1));
            as->and_(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x & 8) - 14) & 3) == 2
        case 19: {
            as->and_(x, zasm::Imm(8));
            as->sub(x, zasm::Imm(14));
            as->and_(x, zasm::Imm(3));
            as->cmp(x, zasm::Imm(2));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x ^ 6) << 9) & 12) == 0
        case 20: {
            as->xor_(x, zasm::Imm(6));
            as->shl(x, zasm::Imm(9));
            as->and_(x, zasm::Imm(12));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x ^ 8) & 2) & 16) == 0
        case 21: {
            as->xor_(x, zasm::Imm(8));
            as->and_(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(16));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x - 15) << 16) & 6) == 0
        case 22: {
            as->sub(x, zasm::Imm(15));
            as->shl(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(6));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // (((x - 10) & 3) & 12) == 0
        case 23: {
            as->sub(x, zasm::Imm(10));
            as->and_(x, zasm::Imm(3));
            as->and_(x, zasm::Imm(12));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 3) + 4) << 6) & 10) == 0
        case 24: {
            as->add(x, zasm::Imm(3));
            as->add(x, zasm::Imm(4));
            as->shl(x, zasm::Imm(6));
            as->and_(x, zasm::Imm(10));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 1) + 3) & 2) & 12) == 0
        case 25: {
            as->add(x, zasm::Imm(1));
            as->add(x, zasm::Imm(3));
            as->and_(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(12));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 9) << 5) + 1) & 2) == 0
        case 26: {
            as->add(x, zasm::Imm(9));
            as->shl(x, zasm::Imm(5));
            as->add(x, zasm::Imm(1));
            as->and_(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 2) << 15) << 2) & 2) == 0
        case 27: {
            as->add(x, zasm::Imm(2));
            as->shl(x, zasm::Imm(15));
            as->shl(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 6) << 11) & 8) + 2) == 2
        case 28: {
            as->add(x, zasm::Imm(6));
            as->shl(x, zasm::Imm(11));
            as->and_(x, zasm::Imm(8));
            as->add(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(2));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 4) << 2) & 3) << 10) == 0
        case 29: {
            as->add(x, zasm::Imm(4));
            as->shl(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(3));
            as->shl(x, zasm::Imm(10));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 1) << 1) & 1) >> 16) == 0
        case 30: {
            as->add(x, zasm::Imm(1));
            as->shl(x, zasm::Imm(1));
            as->and_(x, zasm::Imm(1));
            as->shr(x, zasm::Imm(16));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 4) << 16) & 2) & 7) == 0
        case 31: {
            as->add(x, zasm::Imm(4));
            as->shl(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(7));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 2) << 16) & 14) ^ 15) == 15
        case 32: {
            as->add(x, zasm::Imm(2));
            as->shl(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(14));
            as->xor_(x, zasm::Imm(15));
            as->cmp(x, zasm::Imm(15));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 15) << 4) ^ 15) & 2) == 2
        case 33: {
            as->add(x, zasm::Imm(15));
            as->shl(x, zasm::Imm(4));
            as->xor_(x, zasm::Imm(15));
            as->and_(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(2));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 7) >> 1) << 8) & 6) == 0
        case 34: {
            as->add(x, zasm::Imm(7));
            as->shr(x, zasm::Imm(1));
            as->shl(x, zasm::Imm(8));
            as->and_(x, zasm::Imm(6));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 1) >> 12) & 2) & 4) == 0
        case 35: {
            as->add(x, zasm::Imm(1));
            as->shr(x, zasm::Imm(12));
            as->and_(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(4));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 6) & 8) + 8) & 3) == 0
        case 36: {
            as->add(x, zasm::Imm(6));
            as->and_(x, zasm::Imm(8));
            as->add(x, zasm::Imm(8));
            as->and_(x, zasm::Imm(3));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 3) & 3) << 8) & 10) == 0
        case 37: {
            as->add(x, zasm::Imm(3));
            as->and_(x, zasm::Imm(3));
            as->shl(x, zasm::Imm(8));
            as->and_(x, zasm::Imm(10));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 10) & 2) & 12) + 1) == 1
        case 38: {
            as->add(x, zasm::Imm(10));
            as->and_(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(12));
            as->add(x, zasm::Imm(1));
            as->cmp(x, zasm::Imm(1));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 8) & 2) & 8) << 12) == 0
        case 39: {
            as->add(x, zasm::Imm(8));
            as->and_(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(8));
            as->shl(x, zasm::Imm(12));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 1) & 1) & 16) >> 16) == 0
        case 40: {
            as->add(x, zasm::Imm(1));
            as->and_(x, zasm::Imm(1));
            as->and_(x, zasm::Imm(16));
            as->shr(x, zasm::Imm(16));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 15) & 3) & 2) & 1) == 0
        case 41: {
            as->add(x, zasm::Imm(15));
            as->and_(x, zasm::Imm(3));
            as->and_(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(1));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 4) & 1) & 12) ^ 8) == 8
        case 42: {
            as->add(x, zasm::Imm(4));
            as->and_(x, zasm::Imm(1));
            as->and_(x, zasm::Imm(12));
            as->xor_(x, zasm::Imm(8));
            as->cmp(x, zasm::Imm(8));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 8) & 4) ^ 8) & 1) == 0
        case 43: {
            as->add(x, zasm::Imm(8));
            as->and_(x, zasm::Imm(4));
            as->xor_(x, zasm::Imm(8));
            as->and_(x, zasm::Imm(1));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 6) ^ 3) << 16) & 6) == 0
        case 44: {
            as->add(x, zasm::Imm(6));
            as->xor_(x, zasm::Imm(3));
            as->shl(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(6));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 2) ^ 2) & 3) & 12) == 0
        case 45: {
            as->add(x, zasm::Imm(2));
            as->xor_(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(3));
            as->and_(x, zasm::Imm(12));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 1) - 8) << 16) & 6) == 0
        case 46: {
            as->add(x, zasm::Imm(1));
            as->sub(x, zasm::Imm(8));
            as->shl(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(6));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x + 1) - 2) & 2) & 4) == 0
        case 47: {
            as->add(x, zasm::Imm(1));
            as->sub(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(4));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 2) + 2) + 1) & 3) == 3
        case 48: {
            as->shl(x, zasm::Imm(2));
            as->add(x, zasm::Imm(2));
            as->add(x, zasm::Imm(1));
            as->and_(x, zasm::Imm(3));
            as->cmp(x, zasm::Imm(3));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 2) + 1) << 1) & 2) == 2
        case 49: {
            as->shl(x, zasm::Imm(2));
            as->add(x, zasm::Imm(1));
            as->shl(x, zasm::Imm(1));
            as->and_(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(2));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 6) + 15) & 14) + 4) == 18
        case 50: {
            as->shl(x, zasm::Imm(6));
            as->add(x, zasm::Imm(15));
            as->and_(x, zasm::Imm(14));
            as->add(x, zasm::Imm(4));
            as->cmp(x, zasm::Imm(18));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 9) + 3) & 7) << 12) == 12288
        case 51: {
            as->shl(x, zasm::Imm(9));
            as->add(x, zasm::Imm(3));
            as->and_(x, zasm::Imm(7));
            as->shl(x, zasm::Imm(12));
            as->cmp(x, zasm::Imm(12288));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 2) + 1) & 2) >> 16) == 0
        case 52: {
            as->shl(x, zasm::Imm(2));
            as->add(x, zasm::Imm(1));
            as->and_(x, zasm::Imm(2));
            as->shr(x, zasm::Imm(16));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 9) + 7) & 14) & 6) == 6
        case 53: {
            as->shl(x, zasm::Imm(9));
            as->add(x, zasm::Imm(7));
            as->and_(x, zasm::Imm(14));
            as->and_(x, zasm::Imm(6));
            as->cmp(x, zasm::Imm(6));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 16) + 3) & 3) ^ 2) == 1
        case 54: {
            as->shl(x, zasm::Imm(16));
            as->add(x, zasm::Imm(3));
            as->and_(x, zasm::Imm(3));
            as->xor_(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(1));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 6) + 16) & 16) - 3) == 13
        case 55: {
            as->shl(x, zasm::Imm(6));
            as->add(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(16));
            as->sub(x, zasm::Imm(3));
            as->cmp(x, zasm::Imm(13));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 16) + 3) ^ 1) & 2) == 2
        case 56: {
            as->shl(x, zasm::Imm(16));
            as->add(x, zasm::Imm(3));
            as->xor_(x, zasm::Imm(1));
            as->and_(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(2));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 9) + 8) - 15) & 3) == 1
        case 57: {
            as->shl(x, zasm::Imm(9));
            as->add(x, zasm::Imm(8));
            as->sub(x, zasm::Imm(15));
            as->and_(x, zasm::Imm(3));
            as->cmp(x, zasm::Imm(1));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 16) << 2) + 7) & 3) == 3
        case 58: {
            as->shl(x, zasm::Imm(16));
            as->shl(x, zasm::Imm(2));
            as->add(x, zasm::Imm(7));
            as->and_(x, zasm::Imm(3));
            as->cmp(x, zasm::Imm(3));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 16) << 16) << 16) << 16) == 0
        case 59: {
            as->shl(x, zasm::Imm(16));
            as->shl(x, zasm::Imm(16));
            as->shl(x, zasm::Imm(16));
            as->shl(x, zasm::Imm(16));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 6) << 5) << 14) & 2) == 0
        case 60: {
            as->shl(x, zasm::Imm(6));
            as->shl(x, zasm::Imm(5));
            as->shl(x, zasm::Imm(14));
            as->and_(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 1) << 16) & 13) + 6) == 6
        case 61: {
            as->shl(x, zasm::Imm(1));
            as->shl(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(13));
            as->add(x, zasm::Imm(6));
            as->cmp(x, zasm::Imm(6));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 1) << 3) & 1) << 2) == 0
        case 62: {
            as->shl(x, zasm::Imm(1));
            as->shl(x, zasm::Imm(3));
            as->and_(x, zasm::Imm(1));
            as->shl(x, zasm::Imm(2));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 16) << 16) & 1) >> 16) == 0
        case 63: {
            as->shl(x, zasm::Imm(16));
            as->shl(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(1));
            as->shr(x, zasm::Imm(16));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 16) << 16) & 2) & 1) == 0
        case 64: {
            as->shl(x, zasm::Imm(16));
            as->shl(x, zasm::Imm(16));
            as->and_(x, zasm::Imm(2));
            as->and_(x, zasm::Imm(1));
            as->cmp(x, zasm::Imm(0));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 1) << 4) & 1) ^ 1) == 1
        case 65: {
            as->shl(x, zasm::Imm(1));
            as->shl(x, zasm::Imm(4));
            as->and_(x, zasm::Imm(1));
            as->xor_(x, zasm::Imm(1));
            as->cmp(x, zasm::Imm(1));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        // ((((x << 16) << 16) ^ 15) & 3) == 3
        case 66: {
            as->shl(x, zasm::Imm(16));
            as->shl(x, zasm::Imm(16));
            as->xor_(x, zasm::Imm(15));
            as->and_(x, zasm::Imm(3));
            as->cmp(x, zasm::Imm(3));
            var_alloc.pop(as);
            as->jz(successor_label);
            as->jmp(dead_branch_label);
            break;
        }

        default:
            throw std::runtime_error("gen_predicate: invalid random index");
        };
    }

} // namespace obfuscator::transform_util