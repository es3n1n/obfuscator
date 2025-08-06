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
    template <pe::any_image_t Img>
    class Function {
    public:
        Function(Img* image, const func_parser::function_t& func): parsed_func(func) {
            bb_decomp::Instance<Img> bb_decomp_inst(image, func.rva, func.size);
            setup(bb_decomp_inst, image);
        }

        explicit Function(std::span<std::uint8_t> raw_data): parsed_func(std::nullopt) {
            bb_decomp::Instance<Img> bb_decomp_inst(raw_data);
            setup(bb_decomp_inst);
        }

        ~Function() = default;
        Function(const Function& instance)
            : program(instance.program), assembler(instance.assembler), observer(instance.observer), bb_storage(instance.bb_storage),
              parsed_func(instance.parsed_func), range(instance.range), lru_reg(instance.lru_reg), bb_provider(instance.bb_provider) { }

    private:
        void apply_passes(std::optional<Img*> image = std::nullopt);
        void calc_range();
        void setup(bb_decomp::Instance<Img>& decomp, std::optional<Img*> image = std::nullopt);

    public:
        // A zasm program instance that contains all of our instructions
        //
        std::shared_ptr<zasm::Program> program;
        std::shared_ptr<zasm::x86::Assembler> assembler;
        std::shared_ptr<Observer> observer;

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
        LRUReg<Img> lru_reg;

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

    template <pe::any_image_t Img>
    Function<Img> analyse(Img* image, const func_parser::function_t& function) {
        auto result = Function<Img>(image, function);
        logger::debug("analysis: analysed function {}", function);
        if (auto size = result.range.size(); size < easm::kMaxEntryInstructionSize) {
            throw std::runtime_error(std::format("analysis: Minimal function size is {} bytes, got {}", easm::kMaxEntryInstructionSize, size));
        }
        return result;
    }

    template <pe::any_image_t Img>
    Function<Img> analyse(std::span<std::uint8_t> function_data) {
        return Function<Img>(function_data);
    }
} // namespace analysis
