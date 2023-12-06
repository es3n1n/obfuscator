# I feel sorry for whoever would try to understand this stuff
from numpy import *
from z3 import *

import itertools

sz = 32  # bits
namei = 0


def name():
    global namei
    namei += 1
    return f'v{namei}'


def add_(solver: Solver) -> list:
    x = BitVec(name(), sz)
    return ['{lhs} + {rhs}', x, lambda val: val + x, 'as->add(x, zasm::Imm({rhs}))']


def shl_(solver: Solver) -> list:
    x = BitVec(name(), sz)
    return ['{lhs} << {rhs}', x, lambda val: val << x, 'as->shl(x, zasm::Imm({rhs}))']


def shr_(solver: Solver) -> list:
    x = BitVec(name(), sz)
    return ['{lhs} >> {rhs}', x, lambda val: val >> x, 'as->shr(x, zasm::Imm({rhs}))']


def and_(solver: Solver) -> list:
    x = BitVec(name(), sz)
    return ['{lhs} & {rhs}', x, lambda val: val & x, 'as->and_(x, zasm::Imm({rhs}))']


def or_(solver: Solver) -> list:
    x = BitVec(name(), sz)
    return ['{lhs} | {rhs}', x, lambda val: val | x, 'as->or_(x, zasm::Imm({rhs}))']


def rotl(solver: Solver) -> list:
    x = BitVec(name(), sz)
    return ['rotl({lhs}, {rhs})', x, lambda val: RotateLeft(val, x), 'as->rol(x, zasm::Imm({rhs}))']


def rotr(solver: Solver) -> list:
    x = BitVec(name(), sz)
    return ['rotr({lhs}, {rhs})', x, lambda val: RotateRight(val, x), 'as->ror(x, zasm::Imm({rhs}))']


def xor(solver: Solver) -> list:
    x = BitVec(name(), sz)
    return ['{lhs} ^ {rhs}', x, lambda val: val ^ x, 'as->xor_(x, zasm::Imm({rhs}))']


def sub(solver: Solver) -> list:
    x = BitVec(name(), sz)
    return ['{lhs} - {rhs}', x, lambda val: val - x, 'as->sub(x, zasm::Imm({rhs}))']


operations = [
    add_,
    shl_,
    shr_,
    and_,
    # or_,
    rotl,
    rotr,
    xor,
    sub,
]


def generate_all(num: int = 3) -> None:
    global namei
    for combination in itertools.product(operations, repeat=num):
        s = Solver()
        namei = 0
        startv = BitVec(name(), sz)

        vals = []
        strs = []
        zstrs = []
        lambdas = []

        for operation in combination:
            r = operation(s)
            zstrs.append(r[3])
            lambdas.append(r[2])
            vals.append(r[1])
            strs.append(r[0])

        cur = startv
        for i in range(len(combination)):
            cur = lambdas[i](cur)

        for i in range(len(vals)):
            s.add(vals[i] > 0, vals[i] <= (sz//2))

        x1, x2 = BitVecs('x1 x2', sz)
        s.add(x1 != x2)
        s.add(cur >= 0, cur <= 40960)

        expr1 = substitute(cur, (startv, x1,))
        expr2 = substitute(cur, (startv, x1,))

        if s.check(expr1 != expr2) != unsat:
            continue

        print(f'({num})', cur, '(checking)')

        counter = 0

        while s.check(expr1 == expr2) != unsat:
            counter += 1

            if counter >= 500:
                break

            m = s.model()
            evaluated_vals = []
            for val in vals:
                evaluated_vals.append(m[val].as_long())

            x3, x4 = BitVecs('x3 x4', sz)
            v = cur

            expr3 = substitute(v, (startv, x3,))
            expr4 = substitute(v, (startv, x4,))

            s.push()

            for i in range(len(vals)):
                s.add(vals[i] == evaluated_vals[i])

            if s.check(expr3 != expr4) != sat:
                sssttrrr = 'x'
                for i in range(len(vals)):
                    sssttrrr = strs[i].format(lhs='(' + sssttrrr, rhs=str(evaluated_vals[i])) + ')'

                vv = m.eval(expr3)
                try:
                    result = vv.as_long()
                except:  # noqa
                    with open('./errors.txt', 'a+') as f:
                        f.write(f'{sssttrrr} == {vv}\n')
                    break  # weird

                sssttrrr += ' == ' + str(result)

                try:
                    ev_check = eval(sssttrrr.replace('x', f'uint{sz}(-1)'))
                    for val_to_check in range(0x1337):
                        ev_check = ev_check and eval(sssttrrr.replace('x', f'uint{sz}({val_to_check})'))
                        if not ev_check:
                            break
                except:
                    ev_check = False

                if ev_check:
                    print(f'({num})', sssttrrr, '(matches)')

                    with open('./generated.txt', 'a+') as f:
                        f.write(sssttrrr + '\n')
                    with open('./zasm_generated.cpp', 'a+') as f:
                        f.write('// ' + sssttrrr + '\n')
                        for i in range(len(vals)):
                            f.write(zstrs[i].format(rhs=str(evaluated_vals[i])) + '\n')
                        f.write(f'as->cmp(x, zasm::Imm({result}))\n')
                        f.write('as->jz(successor_label)\n')
                        f.write('as->jmp(dead_branch_label)\n')
                        f.write('\n')

                    break
            s.pop()

            s.add(Not(And(*[vals[i] == evaluated_vals[i] for i in range(len(vals))])))


for N in range(2, 1337):
    generate_all(N)
