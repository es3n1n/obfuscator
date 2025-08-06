#pragma once
#include "analysis/bb_decomp/bb_decomp.hpp"
#include "analysis/common/common.hpp"
#include "analysis/lru_reg/lru_reg.hpp"
#include "easm/misc/misc.hpp"
#include "func_parser/parser.hpp"
#include "observer/observer.hpp"
#include "util/types.hpp"

#include <list>

namespace analysis {
    class Function {
    public:
        Function(cont::ImageBase* image, const func_parser::function_t& func): image_mode(image->mode()), parsed_func(func), lru_reg(LRUReg(image_mode)) {
            bb_decomp::Instance bb_decomp_inst(image, func.rva, func.size);
            setup(bb_decomp_inst, image);
        }

        Function(const cont::ImageMode image_mode, const std::span<std::uint8_t> raw_data)
            : image_mode(image_mode), parsed_func(std::nullopt), lru_reg(LRUReg(image_mode)) {
            /// \fixme @es3n1n: this is wrong
            bb_decomp::Instance bb_decomp_inst(image_mode == cont::ImageMode::X64 ? zasm::MachineMode::AMD64 : zasm::MachineMode::I386, raw_data);
            setup(bb_decomp_inst);
        }

        ~Function() = default;
        Function(const Function& instance)
            : program(instance.program), assembler(instance.assembler), observer(instance.observer), image_mode(instance.image_mode),
              bb_storage(instance.bb_storage), parsed_func(instance.parsed_func), range(instance.range), lru_reg(instance.lru_reg),
              bb_provider(instance.bb_provider) { }

    private:
        void apply_passes(std::optional<cont::ImageBase*> image = std::nullopt);
        void calc_range();
        void setup(bb_decomp::Instance& decomp, std::optional<cont::ImageBase*> image = std::nullopt);

    public:
        // A zasm program instance that contains all of our instructions
        //
        std::shared_ptr<zasm::Program> program;
        std::shared_ptr<zasm::x86::Assembler> assembler;
        std::shared_ptr<Observer> observer;

        ///
        cont::ImageMode image_mode;

        // A list of split basic blocks
        //
        std::shared_ptr<bb_storage_t> bb_storage;

        // Info about the function from the .map/.pdb files
        //
        std::optional<func_parser::function_t> parsed_func;

        // A start/end range of function
        //
        types::range_t range;

        // Least recently used register info
        //
        LRUReg lru_reg;

        // A list of references within the image, key is the instruction and value is RVA
        // it referenced
        //
        std::unordered_map<rva_t, std::list<insn_t*>> image_references;

        // A lookup table with key set to insn rva and value is the ptr to insn info,
        // \fixme: @es3n1n: ptr could be invalid at some point
        //
        std::unordered_map<rva_t, insn_t*> instructions_lookup;

        // BB Provider
        //
        std::shared_ptr<functional_bb_provider_t> bb_provider;
    };

    inline Function analyse(cont::ImageBase* image, const func_parser::function_t& function) {
        auto result = Function(image, function);
        logger::debug("analysis: analysed function {}", function);
        if (auto size = result.range.size(); size < easm::kMaxEntryInstructionSize) {
            throw std::runtime_error(std::format("analysis: Minimal function size is {} bytes, got {}", easm::kMaxEntryInstructionSize, size));
        }
        return result;
    }

    inline auto analyse(const cont::ImageMode image_mode, const std::span<std::uint8_t> function_data) {
        return Function(image_mode, function_data);
    }
} // namespace analysis
