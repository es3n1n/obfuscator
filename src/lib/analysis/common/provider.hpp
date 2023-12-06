#pragma once
#include "common.hpp"

namespace analysis {
    struct functional_bb_provider_t final : bb_provider_t {
    private:
        template <typename Ty>
        using FuncTy = std::optional<std::function<Ty>>;
        using ResultTy = std::optional<std::shared_ptr<bb_t>>;

    public:
        /// \brief Find bb by start VA
        /// \param va virtual address
        /// \param callee basic block callee
        /// \return optional bb ref
        [[nodiscard]] ResultTy find_by_start_va(rva_t va, bb_t* callee) const override {
            assert(find_by_start_va_.has_value());
            return (*find_by_start_va_)(va, callee);
        }

        /// \brief Find bb by start RVA
        /// \param rva relative virtual address
        /// \param callee basic block callee
        /// \return optional bb ref
        [[nodiscard]] ResultTy find_by_start_rva(rva_t rva, bb_t* callee) const override {
            assert(find_by_start_rva_.has_value());
            return (*find_by_start_rva_)(rva, callee);
        }

        /// \brief Find bb by label
        /// \param label label ptr
        /// \param callee basic block callee
        /// \return optional bb ref
        [[nodiscard]] ResultTy find_by_label(const zasm::Label* label, bb_t* callee) const override {
            assert(find_by_label_.has_value());
            return (*find_by_label_)(label, callee);
        }

        /// \brief Acquire bb reference from raw BB ptr
        /// \param ptr basic block ptr
        /// \return optional bb ref
        [[nodiscard]] ResultTy acquire_ref(const bb_t* ptr) const override {
            assert(acquire_ref_.has_value());
            return (*acquire_ref_)(ptr);
        }

        /// \brief Set VA finder callback
        /// \param callback callback
        void set_va_finder(const FuncTy<ResultTy(rva_t, bb_t*)>& callback) noexcept {
            find_by_start_va_ = callback;
        }

        /// \brief Set RVA finder callback
        /// \param callback callback
        void set_rva_finder(const FuncTy<ResultTy(rva_t, bb_t*)>& callback) noexcept {
            find_by_start_rva_ = callback;
        }

        /// \brief Set label finder callback
        /// \param callback callback
        void set_label_finder(const FuncTy<ResultTy(const zasm::Label*, bb_t*)>& callback) noexcept {
            find_by_label_ = callback;
        }

        /// \brief Acquire bb reference from raw PTR
        /// \param callback callback
        void set_ref_acquire(const FuncTy<ResultTy(const bb_t*)>& callback) noexcept {
            acquire_ref_ = callback;
        }

    private:
        /// \brief VA finder callback
        FuncTy<ResultTy(rva_t, bb_t*)> find_by_start_va_ = std::nullopt;
        /// \brief RVA finder callback
        FuncTy<ResultTy(rva_t, bb_t*)> find_by_start_rva_ = std::nullopt;
        /// \brief Label finder callback
        FuncTy<ResultTy(const zasm::Label*, bb_t*)> find_by_label_ = std::nullopt;
        /// \brief Acquire ref callback
        FuncTy<ResultTy(const bb_t*)> acquire_ref_ = std::nullopt;
    };
} // namespace analysis
