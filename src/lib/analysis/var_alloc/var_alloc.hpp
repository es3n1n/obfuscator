#pragma once
#include "analysis/lru_reg/lru_reg.hpp"
#include "pe/pe.hpp"

namespace analysis {
    struct SymVar {
        /// Allocated register
        zasm::Reg reg;
        /// Root of the allocated register
        zasm::Reg root_reg;
        /// Variable bitsize
        zasm::BitSize bit_size;
        /// Stack space that this variable takes (in bytes)
        std::size_t stack_space;

        /// Implicit conversion to Gp so that we can pass this to the assembler methods
        [[nodiscard]] /* implicit */ operator zasm::x86::Gp() const { // NOLINT
            return zasm::x86::Gp{reg.getId()};
        }

        /// Implicit conversion to Reg so that we can pass this as an operand
        [[nodiscard]] /* implicit */ operator zasm::Reg() const { // NOLINT
            return reg;
        }

        /// Implicit conversion to Operand so that we can pass this as an operand
        [[nodiscard]] /* implicit */ operator zasm::Operand() const { // NOLINT
            return {reg};
        }

        /// \brief Convert root register to gp
        /// \return gp instance
        [[nodiscard]] auto root_gp() const {
            return zasm::x86::Gp{root_reg.getId()};
        }
    };

    template <pe::any_image_t Img>
    class VarAlloc {
    public:
        DEFAULT_CT_CTOR_DTOR(VarAlloc);
        DEFAULT_COPY(VarAlloc);
        explicit VarAlloc(LRUReg<Img>* lru_reg): lru_reg_(lru_reg) { }

        /// \brief Get least recently used register as Gp8
        /// \param random should we choose a random register across least recently used registers?
        /// \return SymVar
        [[nodiscard]] SymVar get_gp8_lo(const bool random = false) {
            return filter([this, random]() -> zasm::Reg { return lru_reg_->get_gp8_lo(random); });
        }

        /// \brief Get least recently used register as Gp16
        /// \param random should we choose a random register across least recently used registers?
        /// \return SymVar
        [[nodiscard]] SymVar get_gp16_lo(const bool random = false) {
            return filter([this, random]() -> zasm::Reg { return lru_reg_->get_gp16_lo(random); });
        }

        /// \brief Get least recently used register as Gp32
        /// \param random should we choose a random register across least recently used registers?
        /// \return SymVar
        [[nodiscard]] SymVar get_gp32_lo(const bool random = false) {
            return filter([this, random]() -> zasm::Reg { return lru_reg_->get_gp32_lo(random); });
        }

        /// \brief Get least recently used register as Gp64
        /// \param random should we choose a random register across least recently used registers?
        /// \return SymVar
        [[nodiscard]] SymVar get_gp64(const bool random = false) {
            return filter([this, random]() -> zasm::Reg { return lru_reg_->get_gp64(random); });
        }

        /// \brief Get least recently used register (gp64 for x64 and gp32 for x86)
        /// \param random should we choose a random register across least recently used registers?
        /// \return SymVar
        [[nodiscard]] SymVar get(const bool random = false) {
            return filter([this, random]() -> zasm::Reg { return lru_reg_->get(random); });
        }

        /// \brief Get register with size passed as `bit_size`
        /// \param bit_size register bit size
        /// \param random should we choose a random register across least recently used registers?
        /// \return SymVar
        [[nodiscard]] SymVar get_for_bits(const zasm::BitSize bit_size, const bool random = false) {
            return filter([this, bit_size, random]() -> zasm::Reg { return lru_reg_->get_for_bits(bit_size, random); });
        }

        /// \brief Push flags to stack
        /// \param assembler zasm assembler ptr
        static void push_flags(zasm::x86::Assembler* assembler) {
            /// \fixme @es3n1n: we should track the instructions that affect CF instead
            if constexpr (pe::is_x64_v<Img>) {
                assembler->pushfq();
            } else {
                assembler->pushfd();
            }
        }

        /// \brief Push all used variables on stack
        /// \param assembler zasm assembler ptr
        void push(zasm::x86::Assembler* assembler) const {
            for (const auto& reg_id : registers_in_use_) {
                assembler->push(zasm::x86::Gp(reg_id));
            }
        }

        /// \brief Pop flags from stack
        /// \param assembler zasm assembler ptr
        static void pop_flags(zasm::x86::Assembler* assembler) {
            if constexpr (pe::is_x64_v<Img>) {
                assembler->popfq();
            } else {
                assembler->popfd();
            }
        }

        /// \brief Pop all used variables from stack
        /// \param assembler zasm assembler ptr
        void pop(zasm::x86::Assembler* assembler) const {
            for (auto it = registers_in_use_.rbegin(); it != registers_in_use_.rend(); std::advance(it, 1)) {
                assembler->pop(zasm::x86::Gp(*it));
            }
        }

        /// \brief Clear all used variables
        void clear() {
            stack_space_used_ = 0;
            registers_in_use_.clear();
        }

        /// \brief Estimate how many bytes would we need for storing all the symbolic vars
        /// \return size in bytes
        [[nodiscard]] std::size_t stack_size() const {
            return stack_space_used_;
        }

    private:
        /// \brief Filter out registers that are already in use
        /// \param callback callback that should return allocated reg
        /// \return sym var
        [[nodiscard]] SymVar filter(const std::function<zasm::Reg()>& callback) {
            zasm::Reg result;
            RegID gp_ptr_id;
            do {
                result = callback();
                gp_ptr_id = lru_reg_->to_gp_ptr(result.getId());
            } while (std::ranges::find(registers_in_use_, gp_ptr_id) != std::end(registers_in_use_));

            /// Save the gp ptr as register in use
            registers_in_use_.emplace_back(gp_ptr_id);

            /// Construct the symbolic var
            const auto result_var = SymVar{
                .reg = result,
                .root_reg = zasm::Reg(gp_ptr_id),
                .bit_size = result.getBitSize(lru_reg_->machine_mode()),
                .stack_space = getBitSize(zasm::Reg(gp_ptr_id).getBitSize(lru_reg_->machine_mode())) / CHAR_BIT,
            };

            /// Update the stack space
            stack_space_used_ += result_var.stack_space;

            return result_var;
        }

        /// \brief A list of gp64 registers that we are already using
        std::list<RegID> registers_in_use_;
        /// \brief How many bytes would we need for storing all allocated vars
        std::size_t stack_space_used_ = 0;
        /// \brief LRU registers storage
        LRUReg<Img>* lru_reg_ = nullptr;
    };
} // namespace analysis