#pragma once
#include "obfuscator/transforms/scheduler.hpp"
#include "obfuscator/transforms/transforms/util/bcf.hpp"
#include <es3n1n/common/random.hpp>

namespace obfuscator::transforms {
    namespace detail::anti_ida_decomp {
        /// For some reason, zydis yells at me when i try to serialize it manually :shrug:
        constexpr std::array<std::uint8_t, 4> kEnterStub = {
            0xC8, 0xFF, 0xFF, 0xFF // enter 0xFFFF, 0xFF
        };
    } // namespace detail::anti_ida_decomp

    template <pe::any_image_t Img>
    class DecompBreak final : public BBTransform<Img> {
    public:
        enum Var : std::uint8_t {
            BREAK_IDA = 0,
            BREAK_GHIDRA = 1,
        };

        /// \brief Optional callback that initializes config variables
        void init_config() override {
            this->ida = &this->new_var(Var::BREAK_IDA, "ida", false, TransformConfig::Var::Type::PER_FUNCTION, true);
            this->ghidra = &this->new_var(Var::BREAK_GHIDRA, "ghidra", false, TransformConfig::Var::Type::PER_FUNCTION, true);
        }

        /// \brief Transform zasm node
        /// \param function Routine that it should transform
        /// \param bb BB that it should transform
        void run_on_bb(TransformContext& /*ctx*/, Function<Img>* function, analysis::bb_t* bb) override {
            /// No successors?
            if (bb->successors.empty()) {
                return;
            }

            /// Get the config vars
            const bool ghidra_opt = this->ghidra->value<bool>();
            const bool ida_opt = this->ida->value<bool>();
            assert(ghidra_opt || ida_opt);

            /// Generate an opaque predicate and insert ENTER -1 somewhere over there
            transform_util::generate_bogus_confrol_flow<Img>(
                function, bb,
                [&](analysis::bb_t* new_bb) -> void {
                    /// Set cursor somewhere in the BB (-2 because i don't feel like placing it after the last insn)
                    auto* as = *function->cursor->after(
                        new_bb->node_at(rnd::number<size_t>(static_cast<size_t>(0), std::max(new_bb->size(), static_cast<size_t>(2)) - 2)));

                    /// Make the choice between ghidra and ida
                    Var mode = ida_opt ? Var::BREAK_IDA : Var::BREAK_GHIDRA;
                    if (ida_opt && ghidra_opt) {
                        mode = rnd::or_(Var::BREAK_IDA, Var::BREAK_GHIDRA);
                    }

                    /// Choose the mode and insert needed stuff
                    switch (mode) {
                    case Var::BREAK_IDA: {
                        as->embed(detail::anti_ida_decomp::kEnterStub.data(), detail::anti_ida_decomp::kEnterStub.size());
                        break;
                    }
                    case Var::BREAK_GHIDRA: {
                        auto var_alloc = function->var_alloc();
                        auto var_1 = var_alloc.get(true);
                        auto var_2 = var_alloc.get(true);

                        as->mov(var_1, pe::is_x64_v<Img> ? zasm::Imm(static_cast<std::int64_t>(-1)) : zasm::Imm(static_cast<std::int32_t>(-1L)));
                        as->lea(var_1, easm::ptr<Img>(var_1));
                        as->mov(var_2, easm::ptr<Img>(var_1));
                        new_bb->push_last_N_insns(as, function->bb_provider.get(), 3);
                        break;
                    }
                    default:
                        assert(false);
                        break;
                    }
                },
                [&](zasm::x86::Assembler* assembler, zasm::Label successor_label, zasm::Label dead_branch_label,
                    analysis::VarAlloc<Img>* var_alloc) -> void {
                    transform_util::generate_opaque_predicate(assembler, successor_label, dead_branch_label, var_alloc); //
                });
        }

    private:
        TransformConfig::Var* ida = nullptr;
        TransformConfig::Var* ghidra = nullptr;
    };
} // namespace obfuscator::transforms
