#pragma once
#include <zasm/zasm.hpp>

/*
64	32lo	16lo	8lo
rax	eax	ax	al
rbx	ebx	bx	bl
rcx	ecx	cx	cl
rdx	edx	dx	dl
rsi	esi	si	sil
rdi	edi	di	dil
rbp	ebp	bp	bpl
rsp	esp	sp	spl
r8	r8d	r8w	r8b
r9	r9d	r9w	r9b
r10	r10d	r10w	r10b
r11	r11d	r11w	r11b
r12	r12d	r12w	r12b
r13	r13d	r13w	r13b
r14	r14d	r14w	r14b
r15	r15d	r15w	r15b

In [1]: lines = '''rax^Ieax^Iax^Ial
   ...: rbx^Iebx^Ibx^Ibl
   ...: rcx^Iecx^Icx^Icl
   ...: rdx^Iedx^Idx^Idl
   ...: rsi^Iesi^Isi^Isil
   ...: rdi^Iedi^Idi^Idil
   ...: rbp^Iebp^Ibp^Ibpl
   ...: rsp^Iesp^Isp^Ispl
   ...: r8^Ir8d^Ir8w^Ir8b
   ...: r9^Ir9d^Ir9w^Ir9b
   ...: r10^Ir10d^Ir10w^Ir10b
   ...: r11^Ir11d^Ir11w^Ir11b
   ...: r12^Ir12d^Ir12w^Ir12b
   ...: r13^Ir13d^Ir13w^Ir13b
   ...: r14^Ir14d^Ir14w^Ir14b
   ...: r15^Ir15d^Ir15w^Ir15b'''

In [2]: lines = lines.splitlines()

In [3]: lines = [x.split('\t') for x in lines]

In [4]: for line in lines:
   ...:     print(f'CONVERTER(ZYDIS_REGISTER_{line[1].upper()}, ZYDIS_REGISTER_{line[0].upper()});')
   ...:
 */

/// \note @es3n1n: Would be pretty cool to have something like this in zasm
namespace easm::reg_convert {
    namespace detail {
        inline void throw_exc() {
            throw std::runtime_error("reg_convert: unknown register");
        }
    } // namespace detail

#define CONVERTER(from, to) \
    case from:              \
        return zasm::Reg::Id(to)

    inline zasm::Reg::Id gp32_to_gp64(const zasm::Reg::Id reg_id) {
        switch (static_cast<ZydisRegister_>(reg_id)) {
            CONVERTER(ZYDIS_REGISTER_EAX, ZYDIS_REGISTER_RAX);
            CONVERTER(ZYDIS_REGISTER_EBX, ZYDIS_REGISTER_RBX);
            CONVERTER(ZYDIS_REGISTER_ECX, ZYDIS_REGISTER_RCX);
            CONVERTER(ZYDIS_REGISTER_EDX, ZYDIS_REGISTER_RDX);
            CONVERTER(ZYDIS_REGISTER_ESI, ZYDIS_REGISTER_RSI);
            CONVERTER(ZYDIS_REGISTER_EDI, ZYDIS_REGISTER_RDI);
            CONVERTER(ZYDIS_REGISTER_EBP, ZYDIS_REGISTER_RBP);
            CONVERTER(ZYDIS_REGISTER_ESP, ZYDIS_REGISTER_RSP);
            CONVERTER(ZYDIS_REGISTER_R8D, ZYDIS_REGISTER_R8);
            CONVERTER(ZYDIS_REGISTER_R9D, ZYDIS_REGISTER_R9);
            CONVERTER(ZYDIS_REGISTER_R10D, ZYDIS_REGISTER_R10);
            CONVERTER(ZYDIS_REGISTER_R11D, ZYDIS_REGISTER_R11);
            CONVERTER(ZYDIS_REGISTER_R12D, ZYDIS_REGISTER_R12);
            CONVERTER(ZYDIS_REGISTER_R13D, ZYDIS_REGISTER_R13);
            CONVERTER(ZYDIS_REGISTER_R14D, ZYDIS_REGISTER_R14);
            CONVERTER(ZYDIS_REGISTER_R15D, ZYDIS_REGISTER_R15);
        default:
            break;
        }
        detail::throw_exc();
        std::unreachable();
    }

    inline zasm::Reg::Id gp64_to_gp32(const zasm::Reg::Id reg_id) {
        switch (static_cast<ZydisRegister_>(reg_id)) {
            CONVERTER(ZYDIS_REGISTER_RAX, ZYDIS_REGISTER_EAX);
            CONVERTER(ZYDIS_REGISTER_RBX, ZYDIS_REGISTER_EBX);
            CONVERTER(ZYDIS_REGISTER_RCX, ZYDIS_REGISTER_ECX);
            CONVERTER(ZYDIS_REGISTER_RDX, ZYDIS_REGISTER_EDX);
            CONVERTER(ZYDIS_REGISTER_RSI, ZYDIS_REGISTER_ESI);
            CONVERTER(ZYDIS_REGISTER_RDI, ZYDIS_REGISTER_EDI);
            CONVERTER(ZYDIS_REGISTER_RBP, ZYDIS_REGISTER_EBP);
            CONVERTER(ZYDIS_REGISTER_RSP, ZYDIS_REGISTER_ESP);
            CONVERTER(ZYDIS_REGISTER_R8, ZYDIS_REGISTER_R8D);
            CONVERTER(ZYDIS_REGISTER_R9, ZYDIS_REGISTER_R9D);
            CONVERTER(ZYDIS_REGISTER_R10, ZYDIS_REGISTER_R10D);
            CONVERTER(ZYDIS_REGISTER_R11, ZYDIS_REGISTER_R11D);
            CONVERTER(ZYDIS_REGISTER_R12, ZYDIS_REGISTER_R12D);
            CONVERTER(ZYDIS_REGISTER_R13, ZYDIS_REGISTER_R13D);
            CONVERTER(ZYDIS_REGISTER_R14, ZYDIS_REGISTER_R14D);
            CONVERTER(ZYDIS_REGISTER_R15, ZYDIS_REGISTER_R15D);
        default:
            break;
        }
        detail::throw_exc();
        std::unreachable();
    }

    inline zasm::Reg::Id gp64_to_gp16(const zasm::Reg::Id reg_id) {
        switch (static_cast<ZydisRegister_>(reg_id)) {
            CONVERTER(ZYDIS_REGISTER_RAX, ZYDIS_REGISTER_AX);
            CONVERTER(ZYDIS_REGISTER_RBX, ZYDIS_REGISTER_BX);
            CONVERTER(ZYDIS_REGISTER_RCX, ZYDIS_REGISTER_CX);
            CONVERTER(ZYDIS_REGISTER_RDX, ZYDIS_REGISTER_DX);
            CONVERTER(ZYDIS_REGISTER_RSI, ZYDIS_REGISTER_SI);
            CONVERTER(ZYDIS_REGISTER_RDI, ZYDIS_REGISTER_DI);
            CONVERTER(ZYDIS_REGISTER_RBP, ZYDIS_REGISTER_BP);
            CONVERTER(ZYDIS_REGISTER_RSP, ZYDIS_REGISTER_SP);
            CONVERTER(ZYDIS_REGISTER_R8, ZYDIS_REGISTER_R8W);
            CONVERTER(ZYDIS_REGISTER_R9, ZYDIS_REGISTER_R9W);
            CONVERTER(ZYDIS_REGISTER_R10, ZYDIS_REGISTER_R10W);
            CONVERTER(ZYDIS_REGISTER_R11, ZYDIS_REGISTER_R11W);
            CONVERTER(ZYDIS_REGISTER_R12, ZYDIS_REGISTER_R12W);
            CONVERTER(ZYDIS_REGISTER_R13, ZYDIS_REGISTER_R13W);
            CONVERTER(ZYDIS_REGISTER_R14, ZYDIS_REGISTER_R14W);
            CONVERTER(ZYDIS_REGISTER_R15, ZYDIS_REGISTER_R15W);
        default:
            break;
        }
        detail::throw_exc();
        std::unreachable();
    }

    inline zasm::Reg::Id gp64_to_gp8(const zasm::Reg::Id reg_id) {
        switch (static_cast<ZydisRegister_>(reg_id)) {
            CONVERTER(ZYDIS_REGISTER_RAX, ZYDIS_REGISTER_AL);
            CONVERTER(ZYDIS_REGISTER_RBX, ZYDIS_REGISTER_BL);
            CONVERTER(ZYDIS_REGISTER_RCX, ZYDIS_REGISTER_CL);
            CONVERTER(ZYDIS_REGISTER_RDX, ZYDIS_REGISTER_DL);
            CONVERTER(ZYDIS_REGISTER_RSI, ZYDIS_REGISTER_SIL);
            CONVERTER(ZYDIS_REGISTER_RDI, ZYDIS_REGISTER_DIL);
            CONVERTER(ZYDIS_REGISTER_RBP, ZYDIS_REGISTER_BPL);
            CONVERTER(ZYDIS_REGISTER_RSP, ZYDIS_REGISTER_SPL);
            CONVERTER(ZYDIS_REGISTER_R8, ZYDIS_REGISTER_R8B);
            CONVERTER(ZYDIS_REGISTER_R9, ZYDIS_REGISTER_R9B);
            CONVERTER(ZYDIS_REGISTER_R10, ZYDIS_REGISTER_R10B);
            CONVERTER(ZYDIS_REGISTER_R11, ZYDIS_REGISTER_R11B);
            CONVERTER(ZYDIS_REGISTER_R12, ZYDIS_REGISTER_R12B);
            CONVERTER(ZYDIS_REGISTER_R13, ZYDIS_REGISTER_R13B);
            CONVERTER(ZYDIS_REGISTER_R14, ZYDIS_REGISTER_R14B);
            CONVERTER(ZYDIS_REGISTER_R15, ZYDIS_REGISTER_R15B);
        default:
            break;
        }
        detail::throw_exc();
        std::unreachable();
    }

#undef CONVERTER
} // namespace easm::reg_convert
