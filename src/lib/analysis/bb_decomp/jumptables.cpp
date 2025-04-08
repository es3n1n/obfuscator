#include "analysis/analysis.hpp"
#include "analysis/bb_decomp/bb_decomp.hpp"
#include "analysis/common/debug.hpp"
#include <es3n1n/common/logger.hpp>

/// \todo @es3n1n: Notify the linker somehow that it should erase jumptable pointers too
namespace analysis::bb_decomp {
    template <pe::any_image_t Img>
    void Instance<Img>::collect_jumptables() {
        const auto machine_mode = image_->guess_machine_mode();

        for (auto& basic_block : std::views::values(basic_blocks_)) {
            for (std::size_t i = 0; i < basic_block->size(); ++i) {
                const auto& insn = basic_block->instructions.at(i);

                /// We aren't interested in the successful estimations of jcc/jmps
                if ((insn->flags & UNABLE_TO_ESTIMATE_JCC) == 0) {
                    continue;
                }

                /// OK, we just hit a `jmp/jcc reg`, let's confirm that to be sure
                auto* const jmp_reg = insn->ref->getOperandIf<zasm::Reg>(0);
                if (insn->ref->getOperandCount() != 1 || jmp_reg == nullptr) {
                    continue;
                }

                /// Init the jumptable info
                auto jump_table = jump_table_t{};
                jump_table.bb = basic_block;
                jump_table.jmp_at = basic_block->instructions.begin() + static_cast<std::ptrdiff_t>(i);

                /// Now we need find its table ptr, we are gonna do this by iterating back and
                /// matching the load_index and/or base_move
                for (std::size_t j = i; j != static_cast<std::size_t>(-1); j--) {
                    const auto& prev_insn = basic_block->instructions.at(j);

                    auto match_load_index = [&]() -> void {
                        /// If already found
                        if (jump_table.jump_table_rva.has_value()) {
                            return;
                        }

                        /// Match `mov`
                        if (prev_insn->ref->getMnemonic().value() != ZYDIS_MNEMONIC_MOV) {
                            return;
                        }

                        /// Match the dst reg
                        if (const auto* dst_reg = prev_insn->ref->getOperandIf<zasm::Reg>(0);
                            dst_reg == nullptr || dst_reg->getRoot(machine_mode).getId() != jmp_reg->getRoot(machine_mode).getId()) {
                            return;
                        }

                        /// Found it.

                        /// Now we just have to parse the second operand as Mem and extract the table
                        /// base from it.
                        const auto* mem = prev_insn->ref->getOperandIf<zasm::Mem>(1);
                        if (mem == nullptr) {
                            throw std::runtime_error("analysis: bb didn't have a Mem operand");
                        }

                        /// Yay.
                        jump_table.jump_table_rva = std::make_optional(mem->getDisplacement());
                        jump_table.index_load_at = basic_block->begin() + static_cast<std::ptrdiff_t>(j);
                    };

                    auto match_base_move = [&]() -> void {
                        /// If already found
                        if (jump_table.base_move_at.has_value()) {
                            return;
                        }

                        /// We should match only when we already know the index_move
                        if (!jump_table.index_load_at.has_value()) {
                            return;
                        }

                        /// Check the max allowed distance
                        if (std::distance(basic_block->begin(), *jump_table.index_load_at) - j > 3) {
                            return;
                        }

                        /// Match the lea
                        if (prev_insn->ref->getMnemonic().value() != ZYDIS_MNEMONIC_LEA) {
                            return;
                        }

                        /// Get the dst reg operand
                        const auto* const dst_op = prev_insn->ref->getOperandIf<zasm::Reg>(0);
                        if (auto* const src_op = prev_insn->ref->getOperandIf<zasm::Mem>(1); //
                            dst_op == nullptr || src_op == nullptr) {
                            return;
                        }

                        /// Get the memory operand from index_load
                        const auto* const mem_index_op = (**jump_table.index_load_at)->ref->getOperandIf<zasm::Mem>(1);
                        assert(mem_index_op != nullptr);

                        /// If matches, then yeah we found it
                        if (mem_index_op->getBase().getId() != dst_op->getId()) {
                            return;
                        }

                        jump_table.base_move_at = basic_block->begin() + static_cast<std::ptrdiff_t>(j);
                    };

                    match_load_index();
                    match_base_move();
                    if (jump_table.index_load_at.has_value() && jump_table.base_move_at.has_value()) {
                        break;
                    }
                }

                /// Something's off
                if (!jump_table.jump_table_rva.has_value()) {
                    logger::warn("analysis: discovered possible jump table, however didn't find the base at {:#x}", insn->rva.value_or(0));
                    continue;
                }

                /// Looks legit.
                logger::debug("analysis: discovered possible jump table jmp at {:#x} -> {:#x}", insn->rva.value_or(0), jump_table.jump_table_rva.value());
                jump_tables_[jump_table.jump_table_rva.value()] = jump_table;
            }
        }
    }

    template <pe::any_image_t Img>
    void Instance<Img>::collect_jumptable_entries() {
        /// Now we have to bruteforce the number of entries per table.
        /// I know, i know, it's not the proper solution; however, parsing
        /// the jumptables isn't that trivial of a task and it requires
        /// spending ~waaaay~ more time than i can afford on this task.
        /// We can't just look at the insn before `ja` because there
        /// are multiple possible code generation.
        /// And, with some compilers, the access to the jumptable could
        /// be optimized with some math stuff, which we'd have to handle.
        ///
        /// You probably are thinking that there could be potential
        /// collisions of jumptables and you'd be right!
        /// In order to fix that, we first collect all the tables and
        /// their addresses, then check if the entry we're parsing is
        /// colliding with entries from different jump tables.
        for (auto& [rva, info] : jump_tables_) {
            /// Get the table start
            auto* table = image_->template rva_to_ptr<std::uint32_t>(rva);
            if (table == nullptr) {
                throw std::runtime_error("analysis: unable to find the jump table, huh?");
            }

            /// Let the bruteforce begin
            for (std::size_t i = 0; true; i++) {
                /// Get the entry
                auto entry = table[i];

                /// Check if we hit any collision
                if (i != 0 && jump_tables_.contains(rva + (i * sizeof(entry)))) {
                    break;
                }

                /// Get the entry ptr
                auto ptr = image_->template rva_to_ptr<std::uint8_t>(entry);
                if (!ptr) {
                    break;
                }

                /// Get the section and check if its executable
                if (const auto* section = image_->rva_to_section(entry); //
                    !section->characteristics.cnt_code) {
                    break;
                }

                /// Looks like a valid entry to me.
                /// \todo @es3n1n: Check for function bounds (if its even possible)
                info.entries.emplace_back(entry);
            }

            logger::debug("analysis: got {} entries for a jump table at {:#x}", info.entries.size(), rva);
        }
    }

    template <pe::any_image_t Img>
    void Instance<Img>::expand_jumptables() {
        for (auto& [rva, info] : jump_tables_) {
            /// First, we should replace the
            /// `mov reg, [bla+bla*bla+0x1337]` with `lea reg, [bla+bla*bla]`
            /// then we should erase all the other stuff between it and the jump.
            /// Then, we could easily compare this reg value and jump to the bbs.

            assert(info.index_load_at.has_value());

            /// Obtain the ptr mov operand
            const auto* const pjmp_reg = (**info.index_load_at)->ref->getOperandIf<zasm::Reg>(0);
            const auto* const pmem_op = (**info.index_load_at)->ref->getOperandIf<zasm::Mem>(1);
            if (pmem_op == nullptr || pjmp_reg == nullptr) [[unlikely]] {
                throw std::runtime_error("analysis: unable to obtain mem_op/jmp_reg for jumptable expansion");
            }

            /// Copy
            auto mem_op = *pmem_op;
            auto jmp_reg = zasm::x86::Gp(pjmp_reg->getRoot(image_->guess_machine_mode()).getId()); // eax->rax (just in case)

            /// Remove the imm part (that points to the jump table)
            assert(mem_op.getDisplacement() == rva.as<std::int64_t>());
            mem_op.setBitSize(jmp_reg.getBitSize(image_->guess_machine_mode()));
            mem_op.setDisplacement(0);

            /// Remove the base
            /// \fixme @es3n1n: Do we really need to always remove it though?
            mem_op.setBase(zasm::Reg(zasm::Reg::Id::None));

            /// Change the cursor pos
            auto* const insert_to = (**info.index_load_at)->node_ref->getPrev();
            assembler_->setCursor(insert_to);

            /// Erase other nodes
            for (auto it = *info.index_load_at; it != (*info.bb)->instructions.end(); ++it) {
                auto* const ptr = it->get();
                if (!ptr->rva.has_value()) {
                    continue;
                }

                if (*ptr->rva < *(**info.index_load_at)->rva) {
                    continue;
                }

                /// Destroy the node and mark to be removed
                program_->destroy(ptr->node_ref);
                ptr->flags |= TO_BE_REMOVED;
            }

            /// Destroy the base mov, if needed
            if (info.base_move_at.has_value()) {
                const auto ptr = **info.base_move_at;

                /// Destroy and mark as to be removed
                program_->destroy(ptr->node_ref);
                ptr->flags |= TO_BE_REMOVED;
            }

            /// Emit the lea instead
            assembler_->lea(jmp_reg, mem_op);
            auto* last_node = assembler_->getCursor();
            (void)push_last_instruction(*info.bb);

            /// Current bb that it should treat as predecessor
            auto current_bb = *info.bb;

            /// Clear its old successors list
            current_bb->successors.clear();

            /// Some temporary info about the new virtual bbs
            struct bb_info_t {
                std::shared_ptr<bb_t> ptr = nullptr;
                zasm::Node* first = nullptr;
                zasm::Node* last = nullptr;
                std::size_t count = 0;
            };
            std::vector<bb_info_t> new_bbs = {};

            /// Iterate over the jt entries
            std::size_t index = 0;
            for (auto it = info.entries.begin(); it != info.entries.end(); std::advance(it, 1)) {
                /// Create the new BB for this stuff
                auto new_bb = make_virtual_bb();

                /// Create a label, we'll bind it to where the new BB starts later
                auto label = program_->createLabel();

                /// Compare the index value with current index
                assembler_->setCursor(last_node);
                assembler_->cmp(jmp_reg, zasm::Imm16(index * std::max(mem_op.getScale(), static_cast<uint8_t>(1))));
                auto* first_node = assembler_->getCursor();

                /// JZ
                assembler_->jz(label);

                /// Bind the label
                last_node = assembler_->getCursor();
                assembler_->bind(label);

                /// Save new bb info
                new_bbs.emplace_back(new_bb, first_node, last_node, 2); // \fixme @es3n1n: hardcoded count

                /// Analyse this stuff
                auto successor = make_successor(*it, *current_bb->start_rva);

                /// Store label info
                assembler_->setCursor(last_node->getNext());
                (void)push_last_label(successor);

                /// Save the successor for previous virtual bb
                current_bb->successors.emplace_back(new_bb);

                /// Save the successor
                new_bb->successors.emplace_back(successor);
                new_bb->predecessors.emplace_back(current_bb);

                /// Save the new_bb predecessor
                successor->predecessors.emplace_back(new_bb);

                current_bb = successor;
                index++;
            }

            /// Push new instructions to new bbs
            for (auto it = new_bbs.rbegin(); it != new_bbs.rend(); std::advance(it, 1)) {
                assembler_->setCursor(it->last);
                push_last_N_instruction(it->count, it->ptr);
            }
        }
    }

    PE_DECL_TEMPLATE_CLASSES(Instance);
} // namespace analysis::bb_decomp