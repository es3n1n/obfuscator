#pragma once
#include "mathop/operations/operations.hpp"
#include "util/random.hpp"

#include <functional>

namespace mathop::detail {
    /// \brief Visitor helper
    /// \tparam Ts Visitors
    template <class... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;
    };

    /// \brief An util to simplify the Argument visiting process
    /// \param argument Optional argument that gets passed to the lift function
    /// \param lift_no_arg Executed if argument is not set
    /// \param lift_reg Executed if argument is a register
    /// \param lift_imm Executed if argument is an immediate constant
    inline void lift(const std::optional<Argument>& argument, const std::function<void()>& lift_no_arg, const std::function<void(zasm::x86::Gp)>& lift_reg,
                     const std::function<void(zasm::Imm)>& lift_imm) {
        /// If arg is unset
        if (!argument.has_value()) {
            lift_no_arg();
            return;
        }

        /// Visiting the argument then
        std::visit(overloaded{
                       [&](const zasm::Reg reg) -> void { lift_reg(zasm::x86::Gp(reg.getId())); },
                       [&](const std::int64_t imm) -> void { lift_imm(zasm::Imm(imm)); },
                       [&](const std::int32_t imm) -> void { lift_imm(zasm::Imm(imm)); },
                       [&](const std::int16_t imm) -> void { lift_imm(zasm::Imm(imm)); },
                       [&](const std::int8_t imm) -> void { lift_imm(zasm::Imm(imm)); },
                   },
                   *argument);
    }

    /// \brief Generate a random imm in range of the lhs type
    /// \param lhs Lhs
    /// \return Generated operand
    [[nodiscard]] inline ArgumentImm generate_random_argument_in_range(ArgumentImm lhs) {
        ArgumentImm result;

        /// Probably there's a smarter way how to avoid encoding errors
        std::visit(overloaded{
                       [&](const std::int64_t) -> void { result.emplace<std::int64_t>(rnd::number<std::int32_t>()); },
                       [&](const std::int32_t) -> void { result.emplace<std::int32_t>(rnd::number<std::int16_t>()); },
                       [&](const std::int16_t) -> void { result.emplace<std::int16_t>(rnd::number<std::int8_t>()); },
                       [&](const std::int8_t) -> void { result.emplace<std::int8_t>(rnd::number<std::int8_t>()); },
                   },
                   lhs);
        return result;
    }

    /// \brief Empty callback
    inline auto none = [](auto...) -> void {
    };
} // namespace mathop::detail
