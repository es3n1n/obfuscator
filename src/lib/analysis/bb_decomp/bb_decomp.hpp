#pragma once
#include "analysis/common/common.hpp"
#include "analysis/common/provider.hpp"
#include "pe/pe.hpp"

#include <optional>
#include <ranges>
#include <vector>

namespace analysis::bb_decomp {
    /// \brief BB Decomposition instance
    /// \tparam Img Image
    template <pe::any_image_t Img>
    class Instance {
    public:
        Instance(Img* image, const rva_t rva, const std::optional<std::size_t> function_size = std::nullopt)
            : image_(image), function_start_(rva), function_size_(function_size), program_(std::make_shared<zasm::Program>(image->guess_machine_mode())),
              assembler_(std::make_shared<zasm::x86::Assembler>(*program_)), decoder_(easm::Decoder(image_->guess_machine_mode())),
              bb_provider_(std::make_shared<functional_bb_provider_t>()) {
            collect();
        }
        ~Instance() = default;

        Instance(const Instance& instance)
            : image_(instance.image_), function_start_(instance.function_start_), function_size_(instance.function_size_),
              basic_blocks_(instance.basic_blocks_), program_(std::move(instance.program_)), assembler_(std::move(instance.assembler_)),
              decoder_(instance.decoder_), jump_tables_(instance.jump_tables_), bb_provider_(instance.bb_provider_) { }

        void collect();
        void split();
        void sanitize();

        [[maybe_unused]] void dump(); // to stdout
        [[maybe_unused]] void dump_to_visualizer(); // to visualizer script dir

        void clear() noexcept {
            basic_blocks_.clear();
        }

        [[nodiscard]] std::vector<std::shared_ptr<bb_t>> export_raw_blocks() const {
            auto result = basic_blocks_ | std::ranges::views::values | std::ranges::to<std::vector>();
            result.insert(result.end(), virtual_basic_blocks_.begin(), virtual_basic_blocks_.end());
            return result;
        }

        [[nodiscard]] std::shared_ptr<bb_storage_t> export_blocks() const {
            return std::make_shared<bb_storage_t>(export_raw_blocks());
        }

        [[nodiscard]] std::shared_ptr<zasm::Program> export_program() {
            return std::move(program_);
        }

    private:
        std::shared_ptr<bb_t> process_bb(rva_t rva);
        void update_refs();
        void insert_jmps();
        void update_tree();
        void update_rescheduled_cf();

        // jumptables shenainigans
        void collect_jumptables();
        void collect_jumptable_entries();
        void expand_jumptables();

        std::shared_ptr<bb_t> make_successor(const rva_t successor, bb_t* predecessor) {
            auto predecssor_ref = bb_provider_->acquire_ref(predecessor);
            assert(predecssor_ref.has_value());
            return make_successor(successor, predecssor_ref.value());
        }

        std::shared_ptr<bb_t> make_successor(const rva_t successor, const rva_t predecessor) {
            return make_successor(successor, at(predecessor));
        }

        std::shared_ptr<bb_t> make_successor(const rva_t successor, const std::shared_ptr<bb_t>& predecessor) {
            // Don't analyse already existing bbs
            //
            if (seen_bb(successor)) {
                auto successor_ptr = at(successor);
                predecessor->push_successor(successor_ptr);
                return successor_ptr;
            }

            // Processing bb
            //
            auto successor_ptr = process_bb(successor);

            // Add ass successor/predecessor
            //
            successor_ptr->push_predecessor(predecessor);
            predecessor->push_successor(successor_ptr);
            return successor_ptr;
        }

        [[nodiscard]] std::shared_ptr<bb_t> make_virtual_bb() {
            auto result = std::make_shared<bb_t>(image_->guess_machine_mode());
            virtual_basic_blocks_.emplace_back(result);
            return result;
        }

        [[nodiscard]] bool seen_bb(const rva_t rva) const {
            return basic_blocks_.contains(rva);
        }

        [[nodiscard]] bool is_rva_oob(const rva_t rva) const {
            if (!function_size_.has_value()) {
                return false;
            }

            return (rva - function_start_) >= function_size_;
        }

        [[nodiscard]] std::shared_ptr<bb_t> at(const rva_t rva) {
            if (const auto it = basic_blocks_.find(rva); it != basic_blocks_.end()) {
                return it->second;
            }

            basic_blocks_[rva] = std::make_shared<bb_t>(image_->guess_machine_mode());
            return basic_blocks_[rva];
        }

        void push_last_N_instruction(const std::size_t count, const std::shared_ptr<bb_t>& basic_block) const {
            basic_block->push_last_N_insns(assembler_.get(), bb_provider_.get(), count);
        }

        std::shared_ptr<insn_t> push_last_instruction(const std::shared_ptr<bb_t>& basic_block, const std::optional<rva_t> rva = std::nullopt,
                                                      const std::optional<std::uint8_t> size = std::nullopt) const {
            return basic_block->push_insn(assembler_->getCursor(), bb_provider_.get(), rva, size);
        }

        std::shared_ptr<label_t> push_last_label(const std::shared_ptr<bb_t>& basic_block) const {
            return basic_block->push_label(assembler_->getCursor(), bb_provider_.get());
        }

        std::shared_ptr<label_t> push_last_label(bb_t* basic_block) const {
            return basic_block->push_label(assembler_->getCursor(), bb_provider_.get());
        }

        const Img* image_ = nullptr;
        rva_t function_start_ = nullptr;
        std::optional<std::size_t> function_size_ = std::nullopt;

        std::unordered_map<rva_t, std::shared_ptr<bb_t>> basic_blocks_ = {};
        std::vector<std::shared_ptr<bb_t>> virtual_basic_blocks_ = {}; // = without the rva

        std::shared_ptr<zasm::Program> program_ = {};
        std::shared_ptr<zasm::x86::Assembler> assembler_ = {};
        easm::Decoder decoder_;

        std::unordered_map<rva_t, jump_table_t> jump_tables_ = {};

        std::shared_ptr<functional_bb_provider_t> bb_provider_ = {};
    };

    template <pe::any_image_t Img>
    std::vector<bb_t> collect(Img* image, const rva_t rva, std::optional<std::size_t> size = std::nullopt) {
        const auto inst = Instance<Img>(image, rva, size);
        return inst.export_blocks();
    }
} // namespace analysis::bb_decomp
