#include "analysis/analysis.hpp"
#include "analysis/common/pass_context.hpp"

#include "analysis/passes/label_references.hpp"
#include "analysis/passes/misc/bb_insn_passes.hpp"

namespace analysis {
    void Function::apply_passes(std::optional<cont::ImageBase*> image) {
        /// \note: @es3n1n:
        ///     for the apply_bb/apply_insn callbacks please check out the file
        ///     `analysis/transforms/misc/bb_insn_passes.hpp`,
        ///     passes that would need to iter bb/insns by themselves should be inserted here

        /// Constructing the pass context
        PassContext ctx = {
            .image_mode = image_mode,
            .image = image,
            .function = this,
        };

        passes::bb_insn_passes_t::apply(ctx);
        passes::label_references_t::apply(ctx);
    }

    void Function::calc_range() {
        // Reset state
        //
        range.start = std::numeric_limits<std::uintptr_t>::max();
        range.end = nullptr;

        // Iterating over instructions and updating range
        //
        bb_storage->iter_insns([this](const insn_t& instruction) -> void {
            if (!instruction.rva.has_value()) {
                return;
            }

            if (instruction.rva < range.start) {
                range.start = *instruction.rva;
            }

            if (instruction.rva > range.end) {
                range.end = *instruction.rva;
            }
        });
    }

    void Function::setup(bb_decomp::Instance& decomp, std::optional<cont::ImageBase*> image) {
        bb_storage = decomp.export_blocks();
        program = decomp.export_program();
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
        if (image.has_value()) {
            bb_provider->set_va_finder([img_base = (*image)->get_image_base(),
                                        provider = bb_provider.get()](const rva_t va, bb_t* callee) -> std::optional<std::shared_ptr<bb_t>> {
                /// Substract base and find by RVA
                return provider->find_by_start_rva(va - img_base, callee); //
            });
        } else {
            bb_provider->set_va_finder([](const rva_t, bb_t*) -> std::optional<std::shared_ptr<bb_t>> {
                assert(false); /// Nameless functions does not have VAs
                return std::nullopt;
            });
        }

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
} // namespace analysis
