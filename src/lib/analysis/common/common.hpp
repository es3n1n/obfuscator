#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <es3n1n/common/logger.hpp>
#include <functional>
#include <iterator>
#include <vector>

#include "easm/easm.hpp"
#include "util/iterators.hpp"
#include "util/structs.hpp"
#include "util/types.hpp"

/// \todo @es3n1n: split this monstrosity to multiple files
namespace analysis {
    using rva_t = types::rva_t;

    struct bb_t;

    // CF direction representation
    //
    struct cf_direction_t {
        enum class e_type : std::uint8_t {
            JCC_CONDITION_MET = 0,
            JCC_CONDITION_NOT_MET,
            JMP
        };

        std::shared_ptr<bb_t> bb;
        e_type type = e_type::JMP;

        /// Set to true if we were unable to find the next node.
        bool rescheduled = false;
        std::optional<rva_t> rescheduled_va = std::nullopt;
    };

    // If instruction contains some data that should be relocated
    //
    struct insn_reloc_t {
        enum class e_type : std::uint8_t {
            NONE = 0,
            HEADER, // reloc from .reloc section
            IP, // reloc [rip+0x1337]
        };

        rva_t imm_rva = nullptr;
        e_type type = e_type::NONE;

        // set only for .reloc relocations, basically offset from instruction start to the offset
        std::optional<std::uint8_t> offset = std::nullopt;
    };

    // Instruction flags repr
    //
    enum e_insn_fl : std::uint8_t {
        UNABLE_TO_ESTIMATE_JCC = (1U << 0U),
        TO_BE_REMOVED = (1U << 1U)
    };

    // CPU Flags
    //
    struct cpu_flags_t {
        bool cf = false; // Carry Flag
        bool pf = false; // Parity Flag
        bool af = false; // Adjust Flag
        bool zf = false; // Zero Flag
        bool sf = false; // Sign Flag
        bool tf = false; // Trap Flag (for single stepping)
        bool if_ = false; // Interrupt Enable Flag
        bool df = false; // Direction Flag
        bool of = false; // Overflow Flag
        bool iopl1 = false; // I/O Privilege Level flag, first bit
        bool iopl2 = false; // I/O Privilege Level flag, second bit
        bool nt = false; // Nested Task Flag
        bool rf = false; // Resume Flag (used to control the processor's response to debug exceptions)
        bool vm = false; // Virtual 8086 Mode Flag
        bool ac = false; // Alignment Check (or Access Control) Flag
        bool vif = false; // Virtual Interrupt Flag
        bool vip = false; // Virtual Interrupt Pending
        bool id = false; // ID flag (can CPUID instruction be used)

        void set(zasm::InstrCPUFlags flags) noexcept {
            namespace CPUFlags = zasm::x86::CPUFlags;
            auto test = [flags](const auto fl) -> bool {
                return (flags & fl) != CPUFlags::None;
            };

            cf = test(CPUFlags::CF);
            pf = test(CPUFlags::PF);
            af = test(CPUFlags::AF);
            zf = test(CPUFlags::ZF);
            sf = test(CPUFlags::SF);
            tf = test(CPUFlags::TF);
            if_ = test(CPUFlags::IF);
            df = test(CPUFlags::DF);
            of = test(CPUFlags::OF);
            iopl1 = test(CPUFlags::IOPL1);
            iopl2 = test(CPUFlags::IOPL2);
            nt = test(CPUFlags::NT);
            rf = test(CPUFlags::RF);
            vm = test(CPUFlags::VM);
            ac = test(CPUFlags::AC);
            vif = test(CPUFlags::VIF);
            vip = test(CPUFlags::VIP);
            id = test(CPUFlags::ID);
        }
    };

    struct bb_t;

    // Instruction repr
    // \todo @es3n1n: Some smart .destroy method that would unlink zasm node, remove relocs, etc
    struct insn_t {
        // RVA to the start of the insn
        //
        std::optional<rva_t> rva = std::nullopt;

        // Instruction info at the moment when we were decoding it
        //
        std::optional<std::uint8_t> length = std::nullopt;

        // A pointer to the instruction and node **in** the program class (the one that we'll encode)
        //
        zasm::Instruction* ref = nullptr;
        zasm::Node* node_ref = nullptr;

        // BB ref
        //
        bb_t* bb_ref = nullptr;

        // If this vector contains stuff then IP could be changed only
        // in the ways that are stored in this vector.
        // If not set the execution is just linear.
        //
        std::vector<cf_direction_t> cf;

        // Reloc info
        //
        insn_reloc_t reloc = {};

        // Internal analysis flags
        //
        std::underlying_type_t<e_insn_fl> flags = 0;

        // CPU Flags
        //
        cpu_flags_t flags_set_0 = {};
        cpu_flags_t flags_set_1 = {};
        cpu_flags_t flags_modified = {};
        cpu_flags_t flags_tested = {};
        cpu_flags_t flags_undefined = {};

        // Util to find first op of type
        //
        template <typename Ty>
        [[nodiscard]] std::optional<std::size_t> find_operand_index_if() const {
            // Iterating over operands and trying to get it as our type
            //
            for (std::size_t i = 0; i < ref->getOperandCount(); ++i) {
                if (!ref->getOperand(i).holds<Ty>()) {
                    continue;
                }

                return std::make_optional<std::size_t>(i);
            }

            return std::nullopt;
        }

        template <typename Ty>
        Ty* find_operand_if() {
            // Obtaining index of the desired operand
            //
            const auto index = find_operand_index_if<Ty>();
            if (!index.has_value()) {
                return nullptr;
            }

            // Returning it as the type that we were looking for
            //
            return ref->getOperandIf<Ty>(index.value());
        }

        [[nodiscard]] bool is_conditional_jump() const {
            return cf.size() > 1;
        }

        [[nodiscard]] bool is_jump() const {
            return !cf.empty();
        }

        [[nodiscard]] std::shared_ptr<bb_t> linear_successor() const;
    };

    struct bb_provider_t {
        virtual ~bb_provider_t() = default;

        /// \brief Find bb by start VA
        /// \param va virtual address
        /// \param callee basic block callee
        /// \return optional bb ref
        [[nodiscard]] virtual std::optional<std::shared_ptr<bb_t>> find_by_start_va(rva_t va, bb_t* callee) const = 0;

        /// \brief Find bb by start RVA
        /// \param rva relative virtual address
        /// \param callee basic block callee
        /// \return optional bb ref
        [[nodiscard]] virtual std::optional<std::shared_ptr<bb_t>> find_by_start_rva(rva_t rva, bb_t* callee) const = 0;

        /// \brief Find bb by label
        /// \param label label ptr
        /// \param callee basic block callee
        /// \return optional bb ref
        [[nodiscard]] virtual std::optional<std::shared_ptr<bb_t>> find_by_label(const zasm::Label* label, bb_t* callee) const = 0;

        /// \brief Acquire bb reference from raw BB ptr
        /// \param ptr basic block ptr
        /// \return optional bb ref
        [[nodiscard]] virtual std::optional<std::shared_ptr<bb_t>> acquire_ref(const bb_t* ptr) const = 0;
    };

    struct label_t {
        // References
        //
        bb_t* bb_ref = nullptr;
        zasm::Label* ref = nullptr;
        zasm::Node* node_ref = nullptr;

        // Info
        //
        zasm::Label::Id id = zasm::Label::Id::Invalid;
    };

    // Basic block representation, a function consists of multiple BBs
    //
    struct bb_t {
        explicit bb_t(const zasm::MachineMode machine_mode): machine_mode(machine_mode) { }

        // Common stuff for the next processings, this looks sketchy
        //
        zasm::MachineMode machine_mode;

        // Basic block start, end RVAs
        //
        std::optional<rva_t> start_rva;
        std::optional<rva_t> end_rva; // does not include the size of last instruction

        // A set of disassembled instructions
        //
        std::vector<std::shared_ptr<insn_t>> instructions;

        // Attached labels
        //
        std::unordered_map<zasm::Label::Id, std::shared_ptr<label_t>> labels;

        // BB Successor - a block that this block can lead to
        // BB Predecessor - a block that always executes before this block
        //
        std::vector<std::shared_ptr<bb_t>> successors;
        std::vector<std::shared_ptr<bb_t>> predecessors;

        // Set to true if we changed the instructions list, you should reset it by yourself
        //
        std::atomic_bool dirty = false;

        // bf Flags
        //
        union {
            struct {
                bool valid:1;
            };

            [[maybe_unused]] std::uint32_t raw = {0};
        } flags;

        // Util methods
        //
        void push_successor(const std::shared_ptr<bb_t>& value) {
            // Inserting only once
            //
            if (std::ranges::find(successors, value) != successors.end()) {
                return;
            }

            successors.emplace_back(value);
        }

        void push_predecessor(const std::shared_ptr<bb_t>& value) {
            // Inserting only once
            //
            if (std::ranges::find(predecessors, value) != predecessors.end()) {
                return;
            }

            predecessors.emplace_back(value);
        }

        std::shared_ptr<label_t> push_label(zasm::Node* label_node_ptr, const bb_provider_t* /*bb_provider*/) {
            // Acquire label ref
            auto* const ref = label_node_ptr->getIf<zasm::Label>();
            if (ref == nullptr) {
                return nullptr;
            }

            // Construct info
            auto it = std::make_shared<label_t>();
            it->ref = ref;
            it->node_ref = label_node_ptr;
            it->bb_ref = this;
            it->id = ref->getId();

            // Save the ptr
            label_node_ptr->setUserData(it.get()); // remember the ptr
            labels[it->id] = it;

            // We are done here
            return it;
        }

        std::shared_ptr<insn_t> push_insn(zasm::Node* insn_node_ptr, const bb_provider_t* bb_provider, const std::optional<rva_t> rva = std::nullopt,
                                          const std::optional<std::uint8_t> size = std::nullopt,
                                          const std::optional<decltype(instructions)::iterator>& at = std::nullopt) {
            assert(bb_provider != nullptr);

            /// We are storing only instructions
            auto* const ref = insn_node_ptr->getIf<zasm::Instruction>();
            if (ref == nullptr) {
                if (insn_node_ptr->holds<zasm::Label>()) { // \todo @es3n1n: issue some sort of warning?
                    push_label(insn_node_ptr, bb_provider);
                }
                return nullptr; // \fixme @es3n1n: this should be handled somehow different
            }

            /// Construct insn struct
            auto it = std::make_shared<insn_t>();
            it->node_ref = insn_node_ptr;
            it->ref = ref;
            it->bb_ref = this;

            /// Save rva/size
            it->rva = rva;
            it->length = size;

            /// Set some stuff from the insn detail
            if (auto res = ref->getDetail(machine_mode); res.hasValue()) {
                auto [set1, set0, modified, tested, undefined] = res->getCPUFlags();
                it->flags_set_0.set(set0);
                it->flags_set_1.set(set1);
                it->flags_modified.set(modified);
                it->flags_tested.set(tested);
                it->flags_undefined.set(undefined);
            }

            /// Fill the CF change info
            /// \todo @es3n1n: We can probably fill the successors/predecessors list from here?
            auto update_cf = [&it, &ref, &bb_provider, this]() -> void {
                if (!easm::is_jcc_or_jmp(*ref)) {
                    return;
                }

                // Trying to decode JCC/JMP address
                //
                const auto [is_conditional, jcc_branch, jcc_branch_label] = easm::follow_jcc_or_jmp(*ref);

                if (!jcc_branch.has_value() && !jcc_branch_label.has_value()) {
                    // This is bad. Probably we just hit jumptable or some other shenanigans like this
                    logger::warn("analysis: unable to follow JCC/JMP at {:#x}", it->rva.value_or(0));
                    it->flags |= UNABLE_TO_ESTIMATE_JCC;
                    return;
                }

                /// Updating cf
                ///
                auto reschedule = [&it](const cf_direction_t::e_type ref_type, const std::optional<rva_t> rescheduled_va = std::nullopt) -> void {
                    auto& ref_it = it->cf.emplace_back();
                    ref_it.type = ref_type;
                    ref_it.rescheduled = true;
                    ref_it.rescheduled_va = rescheduled_va;
                };
                auto push_cf_changer = [&it, reschedule](const cf_direction_t::e_type ref_type, const std::optional<std::shared_ptr<bb_t>>& bb_ref,
                                                         const std::optional<rva_t> va = std::nullopt) -> void {
                    if (!bb_ref.has_value()) {
                        // This is bad. Let's reschedule it
                        logger::warn("analysis: unable to follow JCC/JMP at {:#x} #2 --> rescheduling", it->rva.value_or(0));
                        reschedule(ref_type, va);
                        return;
                    }

                    auto& ref_it = it->cf.emplace_back();
                    ref_it.type = ref_type;
                    ref_it.bb = *bb_ref;
                };

                /// Push ref if condition not met
                ///
                auto push_not_met = [is_conditional, it, bb_provider, push_cf_changer, reschedule, this]() -> void {
                    if (!is_conditional) {
                        return;
                    }

                    if (it->rva.has_value() && it->length.has_value()) {
                        push_cf_changer(cf_direction_t::e_type::JCC_CONDITION_NOT_MET, bb_provider->find_by_start_rva(*it->rva + *it->length, this));
                        return;
                    }

                    auto* next_node = it->node_ref->getNext();
                    while (next_node != nullptr && !next_node->holds<zasm::Instruction>()) {
                        next_node = next_node->getNext();
                    }

                    if (next_node == nullptr) {
                        reschedule(cf_direction_t::e_type::JCC_CONDITION_NOT_MET);
                        return;
                    }

                    /// \todo @es3n1n: Make sure that this insn is at the very start of the basic block
                    const auto* next_insn = next_node->getUserData<insn_t>();

                    if (next_insn == nullptr) [[unlikely]] {
                        reschedule(cf_direction_t::e_type::JCC_CONDITION_NOT_MET);
                        return;
                    }

                    const auto next_bb_acquired = bb_provider->acquire_ref(next_insn->bb_ref);
                    push_cf_changer(cf_direction_t::e_type::JCC_CONDITION_NOT_MET, next_bb_acquired);
                };
                push_not_met();

                /// Push ref if condition met
                ///
                const auto met_type = is_conditional ? cf_direction_t::e_type::JCC_CONDITION_MET : cf_direction_t::e_type::JMP;

                if (jcc_branch.has_value()) {
                    push_cf_changer(met_type, bb_provider->find_by_start_va(jcc_branch.value(), this));
                    return;
                }

                if (jcc_branch_label.has_value()) {
                    auto bb_ref = bb_provider->find_by_label(jcc_branch_label.value(), this);
                    assert(bb_ref.has_value()); // we shouldn't reschedule this one
                    push_cf_changer(met_type, bb_ref);
                }
            };
            update_cf();

            /// Save the node
            insn_node_ptr->setUserData(it.get()); // remember the ptr
            instructions.emplace(at.value_or(instructions.end()), it);
            dirty = true;
            return it;
        }

        void push_last_N_insns(const zasm::x86::Assembler* assembler, const bb_provider_t* bb_provider, const std::size_t count) {
            /// Get the last inserted instruction
            ///
            auto* node = assembler->getCursor();

            /// Insert in reverse order
            ///
            std::size_t i = 0;
            for (; i < count && node != nullptr; i++, node = node->getPrev()) {
                (void)push_insn(node, bb_provider);
            }

            /// Reverse the newly inserted nodes
            ///
            auto iter = instructions.begin();
            std::advance(iter, count - i);
            std::reverse(iter, instructions.end());
        }

        [[nodiscard]] std::shared_ptr<insn_t> last_non_jmp_insn(zasm::Program* program = nullptr, const bool destroy_jmps = false,
                                                                const bool include_conditional_jmps = false) const {
            auto insns = temp_insns_copy();
            for (auto it = insns.rbegin(); it != insns.rend(); std::advance(it, 1)) {
                auto insn = *it;
                bool skip = false;

                if (include_conditional_jmps && insn->is_conditional_jump()) {
                    skip = true;
                }

                if (insn->is_jump() && !insn->is_conditional_jump()) {
                    skip = true;
                }

                if (skip) {
                    if (destroy_jmps) {
                        assert(program != nullptr);
                        program->destroy(insn->node_ref);
                    }

                    continue;
                }

                return insn;
            }

            throw std::runtime_error(std::format("last_non_jmp_insn: unable to query ({})", static_cast<int>(include_conditional_jmps)));
        }

        [[nodiscard]] bool contains_label(const zasm::Label::Id label_id) const {
            return labels.contains(label_id);
        }

        void clear() {
            // Clearing all BB state
            //
            flags.valid = false;
            start_rva = nullptr;
            end_rva = nullptr;
            successors.clear();
            predecessors.clear();
            instructions.clear();
        }

        void verify_ranges() const {
            if (!start_rva.has_value()) {
                // nothing to verify
                return;
            }

            auto addr = *start_rva;
            for (const auto& instruction : instructions) {
                if (!instruction->rva.has_value()) {
                    // skip newly added instructions (probably expanded jumptable)
                    break;
                }

                if (*instruction->rva != addr) {
                    throw std::runtime_error("bb_analysis: Invalid BB range found!");
                }

                addr = addr + *instruction->length;
            }
        }

        void update_ranges(const bool verify_ranges = false) {
            // If there's nothing to update
            //
            flags.valid = !instructions.empty();
            if (!flags.valid) {
                clear();
                return;
            }

            // Calculating start/end ranges
            //
            std::optional<memory::address> start_rva_ = std::nullopt;
            std::optional<memory::address> end_rva_ = std::nullopt;

            for (const auto& insn : instructions) {
                // skip instructions without rva
                if (!insn->rva.has_value()) {
                    continue;
                }

                // otherwise update start/end
                //
                if (!start_rva_.has_value() || *insn->rva < *start_rva_) {
                    start_rva_ = *insn->rva;
                }

                if (!end_rva_.has_value() || *insn->rva > *end_rva_) {
                    end_rva_ = *insn->rva;
                }
            }

            // Update the range within the struct
            //
            assert(start_rva_.has_value() && end_rva_.has_value());
            start_rva = *start_rva_;
            end_rva = *end_rva_;

            // Verifying ranges just in case
            //
            if (verify_ranges) {
                this->verify_ranges();
            }
        }

        [[nodiscard]] std::vector<std::shared_ptr<insn_t>> temp_insns_copy() const {
            return instructions;
        }

        [[nodiscard]] std::size_t size() const noexcept {
            return instructions.size();
        }

        [[nodiscard]] zasm::Node* node_at(const std::size_t i) const {
            assert(i < instructions.size());
            return instructions.at(i)->node_ref;
        }

        [[maybe_unused]] void update_addresses() const {
            if (!start_rva.has_value()) {
                // no info :(
                return;
            }

            // Address of the first instruction in BB should be the address
            //
            rva_t addr = *start_rva;

            // Iterating over instructions and updating their addresses
            //
            for (const auto& instruction : instructions) {
                // Skip instructions without rva
                //
                if (!instruction->rva.has_value() || !instruction->length.has_value()) {
                    continue;
                }

                instruction->rva = addr;
                addr = addr + *instruction->length;
            }
        }

        auto begin() {
            return instructions.begin();
        }

        auto begin() const {
            return instructions.begin();
        }

        auto end() {
            return instructions.end();
        }

        auto end() const {
            return instructions.end();
        }
    };

    /// \fixme @es3n1n: MOVE THIS STUFF TO .cpp!
    [[nodiscard]] inline std::shared_ptr<bb_t> insn_t::linear_successor() const {
        if (is_jump()) {
            const auto it = std::ranges::find_if(cf, [](auto& pred) -> bool {
                return pred.type == cf_direction_t::e_type::JCC_CONDITION_NOT_MET || pred.type == cf_direction_t::e_type::JMP;
            });
            return it->bb;
        }
        assert(!bb_ref->successors.empty());
        /// \todo @es3n1n: Check if last insn within the BB, return next insn if so
        return bb_ref->successors.at(0);
    }

    // Jump table representation
    //
    struct jump_table_t {
        using insn_ptr_t = std::vector<std::shared_ptr<insn_t>>::iterator;
        std::optional<std::shared_ptr<bb_t>> bb = std::nullopt;

        std::optional<insn_ptr_t> index_load_at = std::nullopt; // mov reg, [bla+bla*bla]
        std::optional<insn_ptr_t> base_move_at = std::nullopt; // insn that goes before index_load and that uses register from index_load insn
        std::optional<insn_ptr_t> jmp_at = std::nullopt; // jmp reg

        std::optional<memory::address> jump_table_rva = std::nullopt;
        std::vector<rva_t> entries;
    };

    struct bb_storage_t {
        DEFAULT_CT_CTOR_DTOR(bb_storage_t);
        DEFAULT_COPY(bb_storage_t);
        explicit bb_storage_t(const std::vector<std::shared_ptr<bb_t>>& value): basic_blocks(value) { }

        using iterator = util::DerefSharedPtrIter<bb_t>;
        using const_iterator = util::DerefSharedPtrIter<const bb_t>;

        [[nodiscard]] auto& emplace_back() {
            return basic_blocks.emplace_back(std::make_shared<bb_t>(basic_blocks.begin()->get()->machine_mode));
        }

        void iter_bbs(const std::function<void(bb_t&)>& callback) {
            std::ranges::for_each(basic_blocks, [callback](const std::shared_ptr<bb_t>& value) -> void { callback(*value); });
        }

        void iter_insns(const std::function<void(insn_t&)>& callback) {
            iter_bbs([&callback](bb_t& basic_block) -> void { //
                std::ranges::for_each(basic_block.instructions, [&callback](auto& ptr) -> void { callback(*ptr); });
            });
        }

        /// Don't forget to stop the observer, i guess? (fixme)
        [[nodiscard]] std::shared_ptr<bb_t> copy_bb(const std::shared_ptr<bb_t>& bb, zasm::x86::Assembler* as, zasm::Program* program,
                                                    const bb_provider_t* provider) {
            /// Alloc bb
            auto new_bb = std::make_shared<bb_t>(bb->machine_mode);
            basic_blocks.emplace_back(new_bb);

            /// Copy all the instructions
            for (const auto& insn : bb->instructions) {
                /// Emit instruction copy, store it in the BB
                as->emit(*insn->ref);
                new_bb->push_insn(as->getCursor(), provider);
            }

            /// Insert jmp if needed
            if (const auto last_insn = bb->instructions.at(bb->size() - 1);
                !last_insn->is_jump() && !last_insn->is_conditional_jump() && !bb->successors.empty()) {
                /// Get the linear successor
                const auto successor = last_insn->linear_successor();

                /// Create new label
                const auto label = program->createLabel();
                auto label_node = program->bindLabel(label);
                assert(label_node.hasValue());

                /// Insert label, remember it
                program->insertBefore(successor->node_at(0), label_node.value());
                successor->push_label(label_node.value(), provider);

                /// Jmp to the successor
                as->jmp(label);
                bb->push_insn(as->getCursor(), provider);
            }

            return new_bb;
        }

        [[nodiscard]] auto begin() {
            return iterator(basic_blocks.begin());
        }

        [[nodiscard]] auto begin() const {
            return const_iterator(basic_blocks.begin());
        }

        [[nodiscard]] auto end() {
            return iterator(basic_blocks.end());
        }

        [[nodiscard]] auto end() const {
            return const_iterator(basic_blocks.end());
        }

        [[nodiscard]] std::size_t size() const {
            return basic_blocks.size();
        }

        [[nodiscard]] std::vector<std::shared_ptr<bb_t>> temp_copy() const {
            return basic_blocks;
        }

        std::vector<std::shared_ptr<bb_t>> basic_blocks;
    };
} // namespace analysis
