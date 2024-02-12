#pragma once
#include <zasm/zasm.hpp>

namespace easm::reg_convert {
    namespace detail {
        inline void throw_exc() {
            throw std::runtime_error("reg_convert: unknown register");
        }
    } // namespace detail

    inline zasm::Reg::Id gp32_to_gp64(const zasm::Reg::Id reg_id) {
        const auto result = zasm::x86::Gp(reg_id).getRoot(zasm::MachineMode::AMD64);
        assert(result.isValid());
        return result.getId();
    }

    inline zasm::Reg::Id gp64_to_gp32(const zasm::Reg::Id reg_id) {
        const auto result = zasm::x86::Gp(reg_id).r32();
        assert(result.isValid());
        return result.getId();
    }

    inline zasm::Reg::Id gp64_to_gp16(const zasm::Reg::Id reg_id) {
        const auto result = zasm::x86::Gp(reg_id).r16();
        assert(result.isValid());
        return result.getId();
    }

    inline zasm::Reg::Id gp64_to_gp8(const zasm::Reg::Id reg_id) {
        const auto result = zasm::x86::Gp(reg_id).r8();
        assert(result.isValid());
        return result.getId();
    }
} // namespace easm::reg_convert
