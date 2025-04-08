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
            bb_storage = bb_decomp_inst.export_blocks();
            program = bb_decomp_inst.export_program();

            calc_range();

            /// Init the bb provider
            bb_provider = std::make_shared<functional_bb_provider_t>();

            /// Set RVA finder
            bb_provider->set_rva_finder([storage = bb_storage.get()](const rva_t rva, bb_t*) -> std::optional<std::shared_ptr<bb_t>> {
                /// Find by RVA
                auto it = std::ranges::find_if(storage->basic_blocks, [rva](auto&& bb) -> bool {
                    return bb->start_rva.has_value() && bb->start_rva.value() == rva; //
                });

                /// Return wrapped in optional
                return it == std::end(storage->basic_blocks) ? std::nullopt : std::make_optional(*it);
            });

            /// Set VA finder
            bb_provider->set_va_finder([img_base = image->raw_image->get_nt_headers()->optional_header.image_base,
                                        provider = bb_provider.get()](const rva_t va, bb_t* callee) -> std::optional<std::shared_ptr<bb_t>> {
                /// Substract base and find by RVA
                return provider->find_by_start_rva(va - img_base, callee); //
            });

            /// Set Label finder
            bb_provider->set_label_finder([storage = bb_storage.get()](const zasm::Label* label, bb_t*) -> std::optional<std::shared_ptr<bb_t>> {
                for (auto& bb : storage->basic_blocks) {
                    /// Continue if bb doesn't contain this label
                    if (!bb->contains_label(label->getId())) {
                        continue;
                    }

                    return bb;
                }

                return std::nullopt;
            });

            /// Set reference acquire callback
            bb_provider->set_ref_acquire([storage = bb_storage.get()](const bb_t* bb) -> std::optional<std::shared_ptr<bb_t>> {
                /// Try to find by ptr
                auto it = std::ranges::find_if(storage->basic_blocks, [bb](const auto& p) -> bool {
                    return p.get() == bb; //
                });

                /// Not found
                if (it == std::end(storage->basic_blocks)) {
                    return std::nullopt;
                }

                /// Found
                return std::make_optional(*it);
            });

            assembler = std::make_shared<zasm::x86::Assembler>(*program);
            observer = std::make_shared<Observer>(program, bb_storage, bb_provider);

            apply_passes(image);
        }

        ~Function() = default;
        Function(const Function& instance)
            : program(instance.program), assembler(instance.assembler), observer(instance.observer), bb_storage(instance.bb_storage),
              parsed_func(instance.parsed_func), range(instance.range), lru_reg(instance.lru_reg), bb_provider(instance.bb_provider) { }

    private:
        void apply_passes(Img* image);
        void calc_range();

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
        func_parser::function_t parsed_func;

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
} // namespace analysis
