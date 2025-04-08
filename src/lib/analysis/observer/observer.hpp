#pragma once
#include "analysis/common/common.hpp"
#include "util/structs.hpp"

#include <regex>
#include <zasm/zasm.hpp>

namespace analysis {
    /// \brief Zasm observer that would update the bb decomp state according to the program modifications
    class Observer final : public zasm::Observer {
    public:
        NON_COPYABLE(Observer);

        /// \brief Init the observer
        /// \param program zasm program that it should be attached to
        /// \param bb_storage already analysed basic block storage
        /// \param bb_provider shared provider instance (should be already initialized)
        explicit Observer(const std::shared_ptr<zasm::Program>& program, const std::shared_ptr<bb_storage_t>& bb_storage,
                          const std::shared_ptr<functional_bb_provider_t>& bb_provider)
            : program_(program), bb_storage_(bb_storage), bb_provider_(bb_provider) {
            program->addObserver(*this);
        }

        /// \brief Shutdown observer
        ~Observer() override {
            program_->removeObserver(*this);
        }

        /// <summary>
        /// This is called before the node is destroyed.
        /// </summary>
        /// <param name="node">The node which will be destroyed</param>
        void onNodeDestroy(zasm::Node* node) override {
            /// Return if stopped
            if (stopped_) {
                return;
            }

            /// Trying to find the node within bb storage
            const auto pair = find_node(node, [](zasm::Node*) -> zasm::Node* { return nullptr; });
            if (!pair.has_value()) {
                throw std::runtime_error("observer: unable to find node [destroy]");
            }

            /// Erase the node
            pair->first->instructions.erase(pair->second);
            pair->first->dirty = true;
        }

        /// <summary>
        /// This is called after a node has been inserted.
        /// </summary>
        /// <param name="node">The node which was inserted</param>
        void onNodeInserted(zasm::Node* node) override {
            /// Return if stopped
            if (stopped_) {
                return;
            }

            /// Try to find prev/next nodes
            auto prev_pair = find_node(node->getPrev(), [](zasm::Node* cur_node) -> zasm::Node* {
                return cur_node->getPrev(); //
            });
            auto next_pair = find_node(node->getNext(), [](zasm::Node* cur_node) -> zasm::Node* {
                return cur_node->getNext(); //
            });

            /// util
            auto insert_to = [this, node](bb_t* bb, const decltype(bb->instructions)::iterator& at) -> void {
                bb->push_insn(node, bb_provider_.get(), std::nullopt, std::nullopt, at);
            };

            /// \fixme @es3n1n:
            /// This logic is slightly wrong. Instead of doing all of this, we have to:
            /// - Find the prev/next nodes
            /// - Check if prev node ends with jcc/jmp
            /// - If not -> insert new node there
            /// - Otherwise check if the newly inserted node is a jcc/jmp
            /// - If yes -> create a new BB for it, update successors for the prev bb
            /// - Otherwise insert it to the next bb
            ///
            /// Also we have to check if newly inserted node is a jcc/jmp and if its inserted
            /// to the middle of the BB, then we should split it.
            /// Hopefully i didn't forget anything

            if (prev_pair.has_value()) {
                /// If it wasn't inserted after the last entry within bb, then we should be good
                /// and we could just insert new node in this bb
                /// If the next pair is unknown, then fuck it just insert it somewhere
                if (auto [bb, insn] = *prev_pair; //
                    ((*insn)->is_jump() && !(*insn)->is_conditional_jump()) || //
                    !next_pair.has_value()) {
                    insert_to(bb, insn + 1); // +1 because we want to insert it **after** the prev item
                    return;
                }
            }

            if (next_pair.has_value()) {
                /// Oh well, just insert it before the `next` insn
                auto [bb, insn] = *next_pair;
                insert_to(bb, insn);
                return;
            }

            throw std::runtime_error("observer: unable to process prev/next nodes [inserted]");
        }

        /// <summary>
        /// This is called after a node has been created.
        /// </summary>
        void onNodeCreated(zasm::Node* /*node*/) override { }

        /// <summary>
        /// This is called before a node is detached.
        /// </summary>
        void onNodeDetach(zasm::Node* /*node*/) override { }

        /// \brief Stop the observer
        void stop() {
            stopped_ = true;
        }

        /// \brief Start the observer
        void start() {
            stopped_ = false;
        }

        /// \brief
        /// \return true - stopped / false - running
        [[nodiscard]] bool stopped() const {
            return stopped_;
        }

    private:
        /// \brief
        /// \param node
        /// \param next
        /// \return
        static std::optional<std::pair<bb_t*, decltype(bb_t::instructions)::iterator>> find_node(zasm::Node* node,
                                                                                                 const std::function<zasm::Node*(zasm::Node*)>& next) {
            /// Looking up prev/next nodes until we hit an insn
            const auto* insn_ref = node->getIf<zasm::Instruction>();
            while (node != nullptr && insn_ref == nullptr) {
                node = next(node);
                if (node != nullptr) {
                    insn_ref = node->getIf<zasm::Instruction>();
                }
            }

            /// No node
            if (node == nullptr || insn_ref == nullptr) {
                return std::nullopt;
            }

            /// Get its analysis insn info
            auto* insn_info = node->getUserData<insn_t>();
            if (insn_info == nullptr || insn_info->bb_ref == nullptr) [[unlikely]] {
                assert(false); // huh?
                std::unreachable();
            }

            /// Return info
            auto iter = std::ranges::find_if(insn_info->bb_ref->instructions, [&](const auto& ptr) -> bool {
                return ptr.get() == insn_info; //
            });
            assert(iter != std::end(insn_info->bb_ref->instructions));

            /// Not found
            return std::make_optional(std::make_pair(insn_info->bb_ref, iter));
        }

        /// \brief Zasm program instance that we're gonna analyze each time
        /// an obfuscation transform creates/destroys something
        std::shared_ptr<zasm::Program> program_;
        /// \brief A reference to the basic block storage
        std::shared_ptr<bb_storage_t> bb_storage_;
        /// \brief BB Provider reference
        std::shared_ptr<functional_bb_provider_t> bb_provider_;
        /// \brief Start/stop
        bool stopped_ = false;
    };
} // namespace analysis
