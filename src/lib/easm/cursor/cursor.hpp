#pragma once
#include "util/structs.hpp"
#include <optional>
#include <zasm/zasm.hpp>

namespace easm {
    enum class node_pos_t : std::uint8_t {
        instead_of = 0,
        before,
        after
    };

    class Cursor {
    public:
        DEFAULT_DTOR(Cursor);
        DEFAULT_COPY(Cursor); // do we need to copy it though?
        explicit Cursor(std::shared_ptr<zasm::Program> program, std::shared_ptr<zasm::x86::Assembler> assembler)
            : program_(std::move(program)), assembler_(std::move(assembler)) { }

        /// \brief Set cursor at the node and destroy it
        /// \param node node that it should destroy
        /// \return assembler ptr
        std::optional<zasm::x86::Assembler*> instead_of(zasm::Node* node) const noexcept {
            if (node == nullptr) {
                return std::nullopt;
            }

            assembler_->setCursor(node);
            program_->destroy(node);
            return std::make_optional(assembler_.get());
        }

        /// \brief Set cursor before the node
        /// \param node node
        /// \return assembler ptr
        std::optional<zasm::x86::Assembler*> before(zasm::Node* node) const noexcept {
            if (node == nullptr) {
                return std::nullopt;
            }

            const auto prev_node = node->getPrev();
            if (prev_node == nullptr) {
                return std::nullopt;
            }

            assembler_->setCursor(prev_node);
            return std::make_optional(assembler_.get());
        }

        /// \brief Set cursor after the node
        /// \param node node
        /// \return assembler ptr
        std::optional<zasm::x86::Assembler*> after(zasm::Node* node) const noexcept {
            if (node == nullptr) {
                return std::nullopt;
            }

            assembler_->setCursor(node);
            return std::make_optional(assembler_.get());
        }

        /// \brief An all-in-one cursor setter
        /// \param node node
        /// \param pos position
        /// \return assembler ptr
        std::optional<zasm::x86::Assembler*> set(zasm::Node* node, const node_pos_t pos = node_pos_t::after) const noexcept {
            switch (pos) {
            case node_pos_t::after:
                return after(node);
            case node_pos_t::before:
                return before(node);
            case node_pos_t::instead_of:
                return instead_of(node);
            }

            std::unreachable();
        }

        /// \brief Set cursor at the program tail (end)
        /// \return assembler ptr
        std::optional<zasm::x86::Assembler*> program_tail() const noexcept {
            assembler_->setCursor(program_->getTail());
            return std::make_optional(assembler_.get());
        }

    private:
        std::shared_ptr<zasm::Program> program_ = {};
        std::shared_ptr<zasm::x86::Assembler> assembler_ = {};
    };
} // namespace easm
