// ((x << 16) & 6) == 0
as->shl(x, zasm::Imm(16))
as->and_(x, zasm::Imm(6))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((x & 16) & 7) == 0
as->and_(x, zasm::Imm(16))
as->and_(x, zasm::Imm(7))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x + 2) << 4) & 6) == 0
as->add(x, zasm::Imm(2))
as->shl(x, zasm::Imm(4))
as->and_(x, zasm::Imm(6))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x + 6) & 3) & 12) == 0
as->add(x, zasm::Imm(6))
as->and_(x, zasm::Imm(3))
as->and_(x, zasm::Imm(12))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x << 9) << 1) & 4) == 0
as->shl(x, zasm::Imm(9))
as->shl(x, zasm::Imm(1))
as->and_(x, zasm::Imm(4))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x << 3) & 4) + 4) == 4
as->shl(x, zasm::Imm(3))
as->and_(x, zasm::Imm(4))
as->add(x, zasm::Imm(4))
as->cmp(x, zasm::Imm(4))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x << 1) & 1) << 9) == 0
as->shl(x, zasm::Imm(1))
as->and_(x, zasm::Imm(1))
as->shl(x, zasm::Imm(9))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x << 16) & 6) & 6) == 0
as->shl(x, zasm::Imm(16))
as->and_(x, zasm::Imm(6))
as->and_(x, zasm::Imm(6))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x << 2) & 2) ^ 2) == 2
as->shl(x, zasm::Imm(2))
as->and_(x, zasm::Imm(2))
as->xor(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(2))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x << 9) ^ 11) & 11) == 11
as->shl(x, zasm::Imm(9))
as->xor(x, zasm::Imm(11))
as->and_(x, zasm::Imm(11))
as->cmp(x, zasm::Imm(11))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x >> 10) << 16) & 6) == 0
as->shr(x, zasm::Imm(10))
as->shl(x, zasm::Imm(16))
as->and_(x, zasm::Imm(6))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x >> 6) & 11) & 4) == 0
as->shr(x, zasm::Imm(6))
as->and_(x, zasm::Imm(11))
as->and_(x, zasm::Imm(4))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x & 1) + 16) & 2) == 0
as->and_(x, zasm::Imm(1))
as->add(x, zasm::Imm(16))
as->and_(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x & 11) << 4) & 6) == 0
as->and_(x, zasm::Imm(11))
as->shl(x, zasm::Imm(4))
as->and_(x, zasm::Imm(6))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x & 4) & 3) + 16) == 16
as->and_(x, zasm::Imm(4))
as->and_(x, zasm::Imm(3))
as->add(x, zasm::Imm(16))
as->cmp(x, zasm::Imm(16))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x & 4) & 2) << 12) == 0
as->and_(x, zasm::Imm(4))
as->and_(x, zasm::Imm(2))
as->shl(x, zasm::Imm(12))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x & 11) & 3) & 12) == 0
as->and_(x, zasm::Imm(11))
as->and_(x, zasm::Imm(3))
as->and_(x, zasm::Imm(12))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x & 2) & 1) ^ 2) == 2
as->and_(x, zasm::Imm(2))
as->and_(x, zasm::Imm(1))
as->xor(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(2))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x & 8) ^ 1) & 2) == 0
as->and_(x, zasm::Imm(8))
as->xor(x, zasm::Imm(1))
as->and_(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x & 8) - 14) & 3) == 2
as->and_(x, zasm::Imm(8))
as->sub(x, zasm::Imm(14))
as->and_(x, zasm::Imm(3))
as->cmp(x, zasm::Imm(2))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x ^ 6) << 9) & 12) == 0
as->xor(x, zasm::Imm(6))
as->shl(x, zasm::Imm(9))
as->and_(x, zasm::Imm(12))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x ^ 8) & 2) & 16) == 0
as->xor(x, zasm::Imm(8))
as->and_(x, zasm::Imm(2))
as->and_(x, zasm::Imm(16))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x - 15) << 16) & 6) == 0
as->sub(x, zasm::Imm(15))
as->shl(x, zasm::Imm(16))
as->and_(x, zasm::Imm(6))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// (((x - 10) & 3) & 12) == 0
as->sub(x, zasm::Imm(10))
as->and_(x, zasm::Imm(3))
as->and_(x, zasm::Imm(12))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 3) + 4) << 6) & 10) == 0
as->add(x, zasm::Imm(3))
as->add(x, zasm::Imm(4))
as->shl(x, zasm::Imm(6))
as->and_(x, zasm::Imm(10))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 1) + 3) & 2) & 12) == 0
as->add(x, zasm::Imm(1))
as->add(x, zasm::Imm(3))
as->and_(x, zasm::Imm(2))
as->and_(x, zasm::Imm(12))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 9) << 5) + 1) & 2) == 0
as->add(x, zasm::Imm(9))
as->shl(x, zasm::Imm(5))
as->add(x, zasm::Imm(1))
as->and_(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 2) << 15) << 2) & 2) == 0
as->add(x, zasm::Imm(2))
as->shl(x, zasm::Imm(15))
as->shl(x, zasm::Imm(2))
as->and_(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 6) << 11) & 8) + 2) == 2
as->add(x, zasm::Imm(6))
as->shl(x, zasm::Imm(11))
as->and_(x, zasm::Imm(8))
as->add(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(2))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 4) << 2) & 3) << 10) == 0
as->add(x, zasm::Imm(4))
as->shl(x, zasm::Imm(2))
as->and_(x, zasm::Imm(3))
as->shl(x, zasm::Imm(10))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 1) << 1) & 1) >> 16) == 0
as->add(x, zasm::Imm(1))
as->shl(x, zasm::Imm(1))
as->and_(x, zasm::Imm(1))
as->shr(x, zasm::Imm(16))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 4) << 16) & 2) & 7) == 0
as->add(x, zasm::Imm(4))
as->shl(x, zasm::Imm(16))
as->and_(x, zasm::Imm(2))
as->and_(x, zasm::Imm(7))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 2) << 16) & 14) ^ 15) == 15
as->add(x, zasm::Imm(2))
as->shl(x, zasm::Imm(16))
as->and_(x, zasm::Imm(14))
as->xor(x, zasm::Imm(15))
as->cmp(x, zasm::Imm(15))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 15) << 4) ^ 15) & 2) == 2
as->add(x, zasm::Imm(15))
as->shl(x, zasm::Imm(4))
as->xor(x, zasm::Imm(15))
as->and_(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(2))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 7) >> 1) << 8) & 6) == 0
as->add(x, zasm::Imm(7))
as->shr(x, zasm::Imm(1))
as->shl(x, zasm::Imm(8))
as->and_(x, zasm::Imm(6))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 1) >> 12) & 2) & 4) == 0
as->add(x, zasm::Imm(1))
as->shr(x, zasm::Imm(12))
as->and_(x, zasm::Imm(2))
as->and_(x, zasm::Imm(4))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 6) & 8) + 8) & 3) == 0
as->add(x, zasm::Imm(6))
as->and_(x, zasm::Imm(8))
as->add(x, zasm::Imm(8))
as->and_(x, zasm::Imm(3))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 3) & 3) << 8) & 10) == 0
as->add(x, zasm::Imm(3))
as->and_(x, zasm::Imm(3))
as->shl(x, zasm::Imm(8))
as->and_(x, zasm::Imm(10))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 10) & 2) & 12) + 1) == 1
as->add(x, zasm::Imm(10))
as->and_(x, zasm::Imm(2))
as->and_(x, zasm::Imm(12))
as->add(x, zasm::Imm(1))
as->cmp(x, zasm::Imm(1))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 8) & 2) & 8) << 12) == 0
as->add(x, zasm::Imm(8))
as->and_(x, zasm::Imm(2))
as->and_(x, zasm::Imm(8))
as->shl(x, zasm::Imm(12))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 1) & 1) & 16) >> 16) == 0
as->add(x, zasm::Imm(1))
as->and_(x, zasm::Imm(1))
as->and_(x, zasm::Imm(16))
as->shr(x, zasm::Imm(16))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 15) & 3) & 2) & 1) == 0
as->add(x, zasm::Imm(15))
as->and_(x, zasm::Imm(3))
as->and_(x, zasm::Imm(2))
as->and_(x, zasm::Imm(1))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 4) & 1) & 12) ^ 8) == 8
as->add(x, zasm::Imm(4))
as->and_(x, zasm::Imm(1))
as->and_(x, zasm::Imm(12))
as->xor(x, zasm::Imm(8))
as->cmp(x, zasm::Imm(8))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 8) & 4) ^ 8) & 1) == 0
as->add(x, zasm::Imm(8))
as->and_(x, zasm::Imm(4))
as->xor(x, zasm::Imm(8))
as->and_(x, zasm::Imm(1))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 6) ^ 3) << 16) & 6) == 0
as->add(x, zasm::Imm(6))
as->xor(x, zasm::Imm(3))
as->shl(x, zasm::Imm(16))
as->and_(x, zasm::Imm(6))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 2) ^ 2) & 3) & 12) == 0
as->add(x, zasm::Imm(2))
as->xor(x, zasm::Imm(2))
as->and_(x, zasm::Imm(3))
as->and_(x, zasm::Imm(12))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 1) - 8) << 16) & 6) == 0
as->add(x, zasm::Imm(1))
as->sub(x, zasm::Imm(8))
as->shl(x, zasm::Imm(16))
as->and_(x, zasm::Imm(6))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x + 1) - 2) & 2) & 4) == 0
as->add(x, zasm::Imm(1))
as->sub(x, zasm::Imm(2))
as->and_(x, zasm::Imm(2))
as->and_(x, zasm::Imm(4))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 2) + 2) + 1) & 3) == 3
as->shl(x, zasm::Imm(2))
as->add(x, zasm::Imm(2))
as->add(x, zasm::Imm(1))
as->and_(x, zasm::Imm(3))
as->cmp(x, zasm::Imm(3))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 2) + 1) << 1) & 2) == 2
as->shl(x, zasm::Imm(2))
as->add(x, zasm::Imm(1))
as->shl(x, zasm::Imm(1))
as->and_(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(2))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 6) + 15) & 14) + 4) == 18
as->shl(x, zasm::Imm(6))
as->add(x, zasm::Imm(15))
as->and_(x, zasm::Imm(14))
as->add(x, zasm::Imm(4))
as->cmp(x, zasm::Imm(18))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 9) + 3) & 7) << 12) == 12288
as->shl(x, zasm::Imm(9))
as->add(x, zasm::Imm(3))
as->and_(x, zasm::Imm(7))
as->shl(x, zasm::Imm(12))
as->cmp(x, zasm::Imm(12288))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 2) + 1) & 2) >> 16) == 0
as->shl(x, zasm::Imm(2))
as->add(x, zasm::Imm(1))
as->and_(x, zasm::Imm(2))
as->shr(x, zasm::Imm(16))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 9) + 7) & 14) & 6) == 6
as->shl(x, zasm::Imm(9))
as->add(x, zasm::Imm(7))
as->and_(x, zasm::Imm(14))
as->and_(x, zasm::Imm(6))
as->cmp(x, zasm::Imm(6))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 16) + 3) & 3) ^ 2) == 1
as->shl(x, zasm::Imm(16))
as->add(x, zasm::Imm(3))
as->and_(x, zasm::Imm(3))
as->xor(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(1))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 6) + 16) & 16) - 3) == 13
as->shl(x, zasm::Imm(6))
as->add(x, zasm::Imm(16))
as->and_(x, zasm::Imm(16))
as->sub(x, zasm::Imm(3))
as->cmp(x, zasm::Imm(13))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 16) + 3) ^ 1) & 2) == 2
as->shl(x, zasm::Imm(16))
as->add(x, zasm::Imm(3))
as->xor(x, zasm::Imm(1))
as->and_(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(2))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 9) + 8) - 15) & 3) == 1
as->shl(x, zasm::Imm(9))
as->add(x, zasm::Imm(8))
as->sub(x, zasm::Imm(15))
as->and_(x, zasm::Imm(3))
as->cmp(x, zasm::Imm(1))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 16) << 2) + 7) & 3) == 3
as->shl(x, zasm::Imm(16))
as->shl(x, zasm::Imm(2))
as->add(x, zasm::Imm(7))
as->and_(x, zasm::Imm(3))
as->cmp(x, zasm::Imm(3))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 16) << 16) << 16) << 16) == 0
as->shl(x, zasm::Imm(16))
as->shl(x, zasm::Imm(16))
as->shl(x, zasm::Imm(16))
as->shl(x, zasm::Imm(16))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 6) << 5) << 14) & 2) == 0
as->shl(x, zasm::Imm(6))
as->shl(x, zasm::Imm(5))
as->shl(x, zasm::Imm(14))
as->and_(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 1) << 16) & 13) + 6) == 6
as->shl(x, zasm::Imm(1))
as->shl(x, zasm::Imm(16))
as->and_(x, zasm::Imm(13))
as->add(x, zasm::Imm(6))
as->cmp(x, zasm::Imm(6))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 1) << 3) & 1) << 2) == 0
as->shl(x, zasm::Imm(1))
as->shl(x, zasm::Imm(3))
as->and_(x, zasm::Imm(1))
as->shl(x, zasm::Imm(2))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 16) << 16) & 1) >> 16) == 0
as->shl(x, zasm::Imm(16))
as->shl(x, zasm::Imm(16))
as->and_(x, zasm::Imm(1))
as->shr(x, zasm::Imm(16))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 16) << 16) & 2) & 1) == 0
as->shl(x, zasm::Imm(16))
as->shl(x, zasm::Imm(16))
as->and_(x, zasm::Imm(2))
as->and_(x, zasm::Imm(1))
as->cmp(x, zasm::Imm(0))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 1) << 4) & 1) ^ 1) == 1
as->shl(x, zasm::Imm(1))
as->shl(x, zasm::Imm(4))
as->and_(x, zasm::Imm(1))
as->xor(x, zasm::Imm(1))
as->cmp(x, zasm::Imm(1))
as->jz(successor_label)
as->jmp(dead_branch_label)

// ((((x << 16) << 16) ^ 15) & 3) == 3
as->shl(x, zasm::Imm(16))
as->shl(x, zasm::Imm(16))
as->xor(x, zasm::Imm(15))
as->and_(x, zasm::Imm(3))
as->cmp(x, zasm::Imm(3))
as->jz(successor_label)
as->jmp(dead_branch_label)

