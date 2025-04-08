#include "analysis/bb_decomp/bb_decomp.hpp"
#include "analysis/common/debug.hpp"
#include <es3n1n/common/logger.hpp>

namespace analysis::bb_decomp {
    template <pe::any_image_t Img>
    void Instance<Img>::collect() {
        // First of all, we should clear the previous results just in case
        //
        clear();

        // Setup the bb provider
        //
        const auto img_base = image_->raw_image->get_nt_headers()->optional_header.image_base;

        // Make successor proxy
        bb_provider_->set_va_finder([this, img_base](const rva_t virt_addr, const bb_t* callee) {
            return make_successor(virt_addr - img_base, callee); //
        });

        // Make successor proxy
        bb_provider_->set_rva_finder([this](const rva_t rva, const bb_t* callee) {
            return make_successor(rva, callee); //
        });

        // Ref acquire
        bb_provider_->set_ref_acquire([this](const bb_t* bb) -> std::optional<std::shared_ptr<bb_t>> {
            /// Try to find in basic_blocks_ first
            auto it = std::ranges::find_if(basic_blocks_, [bb](const auto& pair) -> bool {
                return pair.second.get() == bb; //
            });
            if (it != std::end(basic_blocks_)) {
                return it->second;
            }

            /// Try to find in virtual basic blocks
            auto vit = std::ranges::find_if(virtual_basic_blocks_, [bb](const auto& value) -> bool {
                return value.get() == bb; //
            });
            if (vit != std::end(virtual_basic_blocks_)) {
                return *vit;
            }

            /// Not found :shrug:
            return std::nullopt;
        });

        // Label finder
        bb_provider_->set_label_finder([this](const zasm::Label* label, bb_t*) -> std::optional<std::shared_ptr<bb_t>> {
            // Searching in basic blocks with rvas first
            for (auto& bb : std::views::values(basic_blocks_)) {
                if (!bb->contains_label(label->getId())) {
                    continue;
                }

                return bb;
            }

            // Searching in virtual basic blocks
            for (auto& bb : virtual_basic_blocks_) {
                if (!bb->contains_label(label->getId())) {
                    continue;
                }

                return bb;
            }

            // Not found
            return std::nullopt;
        });

        // Starting with the first basic block, and it will process others automatically
        //
        logger::info("bb_decomp: running phase 1");
        process_bb(function_start_);

        // Expand jumptables
        //
        logger::info("bb_decomp: running phase 2");
        collect_jumptables();
        collect_jumptable_entries();

        // Splitting basic blocks (pt.1)
        //
        logger::info("bb_decomp: running phase 3");
        split();
        update_refs();

        // Expand jumptables and split basic blocks one more time
        //
        logger::info("bb_decomp: running phase 4");
        expand_jumptables();
        update_rescheduled_cf();
        update_refs();
        split();

        // Insert jmps to successors
        //
        logger::info("bb_decomp: running phase 5");
        update_refs();
        insert_jmps();

        // Sanitizing blocks
        //
        logger::info("bb_decomp: running phase 6");
        sanitize();
        update_tree();
        // dump();
    }

    template <pe::any_image_t Img>
    std::shared_ptr<bb_t> Instance<Img>::process_bb(const rva_t rva) {
        // Initialising stuff
        // \fixme: @es3n1n: override `get_nt_headers` in `pe::Image` class
        const std::uint64_t image_base = image_->raw_image->get_nt_headers()->optional_header.image_base;
        const memory::address virtual_address = rva + image_base;
        const std::uint8_t* data_start = image_->rva_to_ptr(static_cast<std::uint32_t>(rva.inner()));

        // Init basic block info
        //
        auto result = at(rva);
        result->flags.valid = true;

        // Initializing state
        //
        std::expected<zasm::InstructionDetail, zasm::Error> insn = std::unexpected(zasm::Error::None);

        // Iterating over BBs instructions
        //
        for (std::size_t offset = 0; !is_rva_oob(rva + offset); offset += insn->getLength()) {
            // Decoding instruction
            //
            insn = decoder_.decode_insn_detail(data_start + offset, easm::kDefaultSize, (virtual_address + offset).inner());
            if (!insn) {
                throw std::runtime_error(std::format("Unable to decode data at {:#x}", rva + offset));
            }

            // Encoding instruction to our program
            //
            if (auto assembler_result = assembler_->emit(insn->getInstruction()); assembler_result != zasm::Error::None) {
                throw std::runtime_error(std::format("Unable to encode decoded data at {:#x} -> {}", rva + offset, static_cast<int>(assembler_result)));
            }

            // Saving instruction to the current BB struct
            //
            const auto insn_desc = push_last_instruction(result, rva + offset, insn->getLength());

            // Asserting reference
            //
            if (insn_desc->ref == nullptr) [[unlikely]] {
                throw std::runtime_error(std::format("Unable to obtain ref to the decoded data at {:#x}", rva + offset));
            }

            // Breaking on `ret` detail
            //
            if (easm::is_ret(*insn)) {
                break;
            }

            // Ignoring anything that wouldn't affect IP
            //
            if (!insn_desc->is_jump() && !(insn_desc->flags & UNABLE_TO_ESTIMATE_JCC)) {
                continue;
            }

            // Ending bb as soon as we hit JCC/JMP
            //
            break;
        }

        // Calculating ranges
        //
        result->update_ranges(true);
        return result;
    }

    template <pe::any_image_t Img>
    void Instance<Img>::update_refs() {
        /// Remove stuff that was marked as to be deleted
        sanitize();

        logger::debug("bb_decomp: updating BB references..");

        /// Collect all the current instructions that should be present within this program
        std::unordered_map<insn_t*, bb_t*> insns = {};
        for (auto& bb : std::views::values(basic_blocks_)) {
            for (auto& insn : *bb) {
                insns[insn.get()] = bb.get();
            }
        }

        /// Sum stats
        std::size_t weird_nodes = 0;

        for (auto* node = program_->getHead(); node != nullptr; node = node->getNext()) {
            auto* const pinsn = node->getUserData<insn_t>();

            // Weird, but ok
            if (pinsn == nullptr) {
                if (node->holds<zasm::Label>() || node->holds<zasm::Instruction>()) {
                    weird_nodes++;
                }

                continue;
            }

            // This is kinda unsafe but whatever..
            pinsn->node_ref = node;
            pinsn->ref = node->getIf<zasm::Instruction>();

            // This node is up2date, we can remove it as we checked it
            if (insns.contains(pinsn)) {
                insns.erase(pinsn);
            }
        }

        // Print stats, if needed
        if (weird_nodes > 0) {
            logger::warn("bb_decomp: got {} weird nodes while updating refs", weird_nodes);
        }

        // Remove oudated nodes that doesn't present in program
        if (!insns.empty()) [[unlikely]] {
            logger::warn("bb_decomp: got {} outdated nodes while updating refs", insns.size());

            for (auto& [insn, bb] : insns) {
                // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
                std::erase_if(bb->instructions, [insn](const std::shared_ptr<insn_t>& item) -> bool { return item.get() == insn; });
            }
        }
    }

    template <pe::any_image_t Img>
    void Instance<Img>::split() {
        /// \note @es3n1n: Looks kinda scary, but splitting 2k+ basic blocks took me ~350ms so
        /// i guess we'll keep it as it is (PR welcome), perhaps an interval/segment tree could be used here
        logger::debug("analysis: splitting BBs..");
        bool split_something; // NOLINT(cppcoreguidelines-init-variables)

        // Splitting while there's something to split
        //
        do {
            // Resetting state
            //
            split_something = false;

            // Iterating over BBs
            //
            for (auto& [address, bb] : basic_blocks_) {
                assert(bb->start_rva.has_value()); // wtf

                // Iterating over other BBs
                //
                for (auto& [address_2, bb_2] : basic_blocks_) {
                    // Skipping the same block
                    //
                    if (address == address_2) {
                        continue;
                    }

                    assert(bb_2->start_rva.has_value()); // wtf

                    // Continue if our block is not a part of the bb_2
                    //
                    if (!(*bb->start_rva >= *bb_2->start_rva && *bb->start_rva <= *bb_2->end_rva)) {
                        continue;
                    }

                    // Mark as split
                    //
                    split_something = true;

                    // Iterating over instructions and shrinking the ones that we already have in our BB
                    //
                    std::erase_if(bb_2->instructions, [this, bb](const std::shared_ptr<insn_t>& insn) -> bool {
                        // Removing `bb` instructions from the `bb_2`
                        //
                        // const bool should_remove = std::ranges::find_if(bb, [insn](const insn_t& insn2) -> bool {
                        //                                return insn.node_ref == insn2.node_ref; // we should remove duplicated stuff
                        //                            }) != std::end(bb);

                        // ignore insns without rvas
                        if (!insn->rva.has_value()) {
                            return false;
                        }

                        const bool should_remove = *insn->rva >= *bb->start_rva && *insn->rva <= *bb->end_rva;

                        // \fixme: @es3n1n: kinda sucks that we have to manually remove nodes, but whatever i guess
                        //
                        if (should_remove) {
                            this->program_->destroy(insn->node_ref);
                        }

                        // Returning result
                        //
                        return should_remove;
                    });

                    // Updating ranges since we modified the instruction set
                    //
                    bb_2->update_ranges(true);

                    // Updating successors of the basic block that contains instructions from the `bb_2`
                    //
                    for (auto& successor : bb_2->successors) {
                        bb->push_successor(successor);
                    }

                    // Updating predecessors because obviously it would contain the `bb_2` now
                    //
                    bb->push_predecessor(bb_2);

                    // Since we merged the successors from this list we can clear it and set to the `bb`
                    //
                    bb_2->successors.clear();
                    bb_2->push_successor(bb);

                    // Exit from loop
                    //
                    break;
                }

                // Exit from loop once we split something
                //
                if (split_something) {
                    break;
                }
            }
        } while (split_something);
    }

    template <pe::any_image_t Img>
    void Instance<Img>::sanitize() {
        const auto erased_bbs = std::erase_if(basic_blocks_, [](auto& basic_block) -> bool {
            const auto nodes_erased = std::erase_if(basic_block.second->instructions, [](auto& insn) -> bool {
                return insn->flags & TO_BE_REMOVED; //
            });

            if (nodes_erased > 0) {
                logger::debug("bb_decomp: sanitized {} nodes", nodes_erased);
            }

            return !basic_block.second->flags.valid;
        });

        if (erased_bbs > 0) {
            logger::debug("bb_decomp: sanitized {} basic blocks", erased_bbs);
        }
    }

    template <pe::any_image_t Img>
    void Instance<Img>::insert_jmps() {
        logger::debug("bb_decomp: veryfing BB intersections..");
        /// Lookup for the basic blocks that for some reason aren't jumping to their successor(s)
        for (auto& bb : std::views::values(basic_blocks_)) {
            if (bb->instructions.empty()) [[unlikely]] {
                continue;
            }

            const auto& last_insn = bb->instructions.at(bb->size() - 1);

            /// Looks legit, i think?
            if (const auto last_mnemonic = last_insn->ref->getMnemonic().value();
                last_mnemonic == ZYDIS_MNEMONIC_JMP || last_mnemonic == ZYDIS_MNEMONIC_RET) {
                continue;
            }

            /// Skip stuff that we added by ourselves
            if (!last_insn->rva.has_value()) {
                continue;
            }

            /// Get the expected next bb
            auto expected_cf_next_bb = std::ranges::find_if(last_insn->cf, [](auto& cf_info) -> bool {
                switch (cf_info.type) {
                case cf_direction_t::e_type::JCC_CONDITION_NOT_MET:
                case cf_direction_t::e_type::JMP:
                    return true;
                default:
                    return false;
                }
            });
            auto* expected_next_bb = expected_cf_next_bb != std::end(last_insn->cf) ? expected_cf_next_bb->bb.get() : nullptr;

            /// If we didn't find it via CF info, then try to get the first successor
            if (expected_next_bb == nullptr) {
                /// Funny case where the compiler just places a call to an exception without any successors or ret instructions
                if (bb->successors.empty()) {
                    continue;
                }

                assert(bb->successors.size() == 1);
                expected_next_bb = bb->successors.at(0).get();
            }

            /// Let's see if we end up on a successor after this node
            zasm::Node* next_node = last_insn->node_ref->getNext();
            while (next_node != nullptr && !next_node->holds<zasm::Instruction>()) {
                next_node = next_node->getNext();
            }

            /// If there's no next node, then we totally should insert a jmp
            if (next_node != nullptr) {
                /// Get the next instruction analysis info
                const auto* const next_insn = next_node->getUserData<insn_t>();
                if (next_insn == nullptr || !next_insn->rva.has_value()) [[unlikely]] {
                    continue;
                }

                /// Get the next instruction bb
                auto next_bb = basic_blocks_.find(next_insn->rva.value());
                if (next_bb == std::end(basic_blocks_)) [[unlikely]] {
                    assert(false); // weird
                    continue;
                }

                /// Verify that the next BB is the expected one
                if (expected_next_bb == next_bb->second.get()) {
                    continue;
                }
            }

            /// We need to adjust this thing otherwise

            /// Create a label
            auto label = program_->createLabel();

            /// Bind this label to the beginning of the next bb
            assembler_->setCursor(expected_next_bb->node_at(0)->getPrev());
            assembler_->bind(label);
            push_last_label(expected_next_bb);

            /// Place the jmp
            assembler_->setCursor(last_insn->node_ref);
            assembler_->jmp(label);
            (void)push_last_instruction(bb);

            /// Theoretically now we should jump where we should?
        }
    }

    template <pe::any_image_t Img>
    void Instance<Img>::update_rescheduled_cf() {
        logger::debug("bb_decomp: updating rescheduled CF..");

        /// Iterating over the all basic blocks
        for (auto& bb : std::views::values(basic_blocks_)) {
            /// Trying to find the insn with CF changers
            const auto& last_insn = bb->instructions.at(bb->size() - 1);

            /// No CF info
            if (last_insn->cf.empty()) {
                continue;
            }

            /// Trying to find rescheduled CF changers
            for (auto& cf_changer : last_insn->cf) {
                /// Not rescheduled
                if (!cf_changer.rescheduled) {
                    continue;
                }

                /// Unset rescheduled flag
                cf_changer.rescheduled = false;
                cf_changer.rescheduled_va = std::nullopt;

                /// Discovering BB by VA
                if (cf_changer.rescheduled_va.has_value()) {
                    auto new_bb = bb_provider_->find_by_start_va(cf_changer.rescheduled_va.value(), bb.get());
                    assert(new_bb.has_value());
                    cf_changer.bb = new_bb.value();
                    continue;
                }

                /// Discovering "next" instruction
                auto* next_node = last_insn->node_ref->getNext();
                while (next_node != nullptr && !next_node->holds<zasm::Instruction>()) {
                    next_node = next_node->getNext();
                }

                assert(next_node != nullptr);

                /// Storing its BB
                const auto* analysis_info = next_node->getUserData<insn_t>();
                auto acquired_ref = bb_provider_->acquire_ref(analysis_info->bb_ref);
                assert(acquired_ref.has_value());
                cf_changer.bb = acquired_ref.value();
            }
        }
    }

    template <pe::any_image_t Img>
    void Instance<Img>::update_tree() {
        logger::debug("bb_decomp: updating the BB tree.. (this could take some time)");

        /// Since we splitted/merged some basic blocks, there could be some
        /// issues since we merged successors/predecessors list and it could
        /// contain some multiple "dead" nodes, that we should update manually

        /// Step 0. Clear all predecessors info
        /// We cannot do this in the Step 1
        for (const auto& bb : std::views::values(basic_blocks_)) {
            bb->predecessors.clear();
        }

        /// Step 1. Updating successors
        for (auto& bb : std::views::values(basic_blocks_)) {
            /// Remove all the "outdated" info
            bb->successors.clear();

            /// Looking for the dead CF changer refs
            /// (because since we're splitting them, the dst bb could've been already deleted at some point)
            for (auto it = bb->instructions.rbegin(); it != bb->instructions.rend(); std::advance(it, 1)) {
                const auto& insn = *it;
                if (insn->cf.empty()) {
                    continue;
                }

                /// Found an instruction that changes the CF, there should be
                /// only 1 such function in BB

                /// Iterating over its entries and updating the BB references
                for (auto& cf_changer : insn->cf) {
                    /// Skip virtual bbs
                    /// \fixme @es3n1n: This could be a problem in the future
                    /// \todo @es3n1n: bb->is_virtual() instead of these checks
                    if (!cf_changer.bb->start_rva.has_value()) {
                        continue;
                    }

                    /// Update the ref
                    auto bb_it = basic_blocks_.find(cf_changer.bb->start_rva.value());
                    if (bb_it == std::end(basic_blocks_)) {
                        throw std::runtime_error("analysis: unable to update CF reference");
                    }
                    cf_changer.bb = bb_it->second;

                    /// Remember this BB as a successor
                    bb->successors.emplace_back(cf_changer.bb);
                }
                break;
            }

            /// If it wasn't already initialized by the CF changer
            if (bb->successors.empty()) {
                /// "Linear" CF

                /// Looking up for the next BBs
                auto* last_node = bb->instructions.at(bb->size() - 1)->node_ref->getNext();
                while (last_node != nullptr && !last_node->holds<zasm::Instruction>()) {
                    last_node = last_node->getNext();
                }

                /// No next instruction :shrug:
                if (last_node == nullptr) {
                    continue;
                }

                /// Get its BB and emplace it as the successor
                const auto* analysis_info = last_node->getUserData<insn_t>();
                auto acquired_bb = bb_provider_->acquire_ref(analysis_info->bb_ref);
                assert(acquired_bb.has_value());
                bb->successors.emplace_back(acquired_bb.value());
            }

            /// Iterating over the successors and updating predecessors in successors
            for (const auto& successor : bb->successors) {
                successor->predecessors.emplace_back(bb);
            }
        }
    }

    template <pe::any_image_t Img>
    void Instance<Img>::dump() {
        logger::info("-- Basic blocks for function {:#x}", function_start_);

        for (auto& v : std::views::values(basic_blocks_)) {
            debug::dump_bb(*v);
        }

        logger::info("-- EOF");
    }

    template <pe::any_image_t Img>
    void Instance<Img>::dump_to_visualizer() {
        const auto path = std::filesystem::path(R"(E:\local-projects\obfuscator\scripts\bb_preview\data\)");
        int iter = 0;

        for (auto& basic_block : std::views::values(basic_blocks_)) {
            debug::serialize_bb_to_file(*basic_block, path / (std::to_string(iter) + ".bb"));
            iter += 1;
        }
    }

    PE_DECL_TEMPLATE_CLASSES(Instance);
} // namespace analysis::bb_decomp
