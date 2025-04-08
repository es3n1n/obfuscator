#include "obfuscator/obfuscator.hpp"
#include "analysis/observer/observer.hpp"
#include "easm/debug/debug.hpp"
#include "obfuscator/config_merger/config_merger.hpp"
#include "obfuscator/function.hpp"
#include "obfuscator/transforms/scheduler.hpp"

#include <es3n1n/common/logger.hpp>
#include <es3n1n/common/progress.hpp>
#include <es3n1n/common/random.hpp>

namespace obfuscator {
    constexpr size_t kTextSectionAlignment = 0x10;

    template <pe::any_image_t Img>
    void Instance<Img>::setup() {
        // Initializing instances
        //
        func_parser_.setup(image_, config_.func_parser_config(), config_.obfuscator_config());

        // Running setup tasks
        //
        func_parser_.collect_functions();

        // Add functions from config, that we should protecc
        //
        auto analysis_progress = progress::Progress("obfuscator: setting up functions", config_.size());
        for (auto& configuration : config_) {
            add_function(configuration);
            analysis_progress.step();
        }

        // Enable transforms from global config
        //
        auto& scheduler = TransformScheduler::get();
        for (auto& [tag, _] : config_.global_transforms_config()) {
            scheduler.enable_transform(tag);
        }
    }

    template <pe::any_image_t Img>
    void Instance<Img>::add_function(const config_parser::function_configuration_t& configuration) {
        /// We don't want to obfuscate functions with 0 transforms
        // if (configuration.transform_configurations.empty()) {
        // logger::warn("collect: excluding function {} from obfuscation list", configuration.function_name);
        // return;
        //}

        /// Try to find function info from map/pdb
        const auto function_info =
            func_parser_.find_if([&configuration](const func_parser::function_t& func) -> bool { return func.name == configuration.function_name; });
        if (!function_info.has_value()) {
            throw std::runtime_error(std::format("collect: function {} not found", configuration.function_name));
        }

        /// Enable needed transforms
        auto& scheduler = TransformScheduler::get();
        for (const auto& [tag, _] : configuration.transform_configurations) {
            scheduler.enable_transform(tag);
        }

        /// Store function info
        functions_.emplace_back(function_t{
            .analysed = analysis::analyse(image_, function_info.value()),
            .configuration = configuration,
        });
    }

    template <pe::any_image_t Img>
    void Instance<Img>::obfuscate() {
        /// Debug log
        logger::info("obfuscator: got {} function(s) to obfuscate", functions_.size());

        if (functions_.empty()) {
            throw std::runtime_error("obfuscator: got 0 functions to protect");
        }

        /// Obtain transform scheduler for the platform
        auto& scheduler = TransformScheduler::get().for_arch<Img>();
        config_merger::apply_global_vars<Img>(config_);

        /// Iterate over functions that we need to obfuscate
        for (const auto& func : functions_) {
            /// Init the `obfuscator::Function` that is going to be used within
            /// transforms
            auto obf_func = obfuscator::Function<Img>(func.analysed, image_);

            /// Export tags that this function would need
            auto tags = std::views::all(func.configuration.transform_configurations) |
                        std::views::transform([](const config_parser::transform_configuration_t& it) -> TransformTag { return it.tag; }) |
                        std::ranges::to<std::vector>();

            /// Export transforms
            auto transforms = scheduler.select_transforms(tags);

            /// Init the progress bar
            auto progress = progress::Progress(std::format("obfuscator: obfuscating {}", obf_func.parsed_func.name), transforms.size());

            /// An util that would check the chances and all this other crap, that would be
            /// needed for like  every possible function/transform
            auto execute_transform = [func](const TransformTag tag, const std::function<void(TransformContext&)>& callback,
                                            const bool check_chances = true) -> void {
                auto preset = std::ranges::find_if(func.configuration.transform_configurations, [tag](auto&& it) -> bool {
                    return it.tag == tag; //
                });
                if (preset == std::end(func.configuration.transform_configurations)) {
                    throw std::runtime_error(std::format("obfuscate: unable to find configuration for transform {}", tag));
                }

                /// Apply the preset
                config_merger::apply_config<Img>(*preset);

                /// Get the shared config and check the chance
                auto& cfg = TransformSharedConfigStorage::get().get_for(tag);

                /// Check the chance
                /// \todo @es3n1n: Check for chance feature
                if (check_chances && !rnd::chance(cfg.chance())) {
                    return;
                }

                /// Otherwise run this method
                for (std::size_t i = 0; i < cfg.repeat_times(); ++i) {
                    /// Init context, run the task
                    auto context = TransformContext(cfg);

                    do {
                        context.rerun_me = false;
                        callback(context);
                    } while (context.rerun_me);
                }
            };
            auto execute_transform_no_chances = [&](const TransformTag tag, const std::function<void(TransformContext&)>& callback) -> void {
                return execute_transform(tag, callback, false);
            };

            /// \note @es3n1n: We can't iterate through the insns/bbs and execute transforms
            /// from there as it would break the scheduling order
            for (auto& [tag, transform] : transforms) {
                /// Apply function transform
                if (transform->feature(TransformFeaturesSet::HAS_FUNCTION_TRANSFORM)) {
                    execute_transform_no_chances(tag, [&obf_func, &transform](auto& ctx) -> void {
                        transform->run_on_function(ctx, &obf_func); //
                    });
                }

                /// Apply basic block transforms
                if (transform->feature(TransformFeaturesSet::HAS_BB_TRANSFORM)) {
                    for (auto& basic_block : obf_func.bb_storage->temp_copy()) {
                        execute_transform(tag, [&obf_func, &transform, &basic_block](auto& ctx) -> void {
                            transform->run_on_bb(ctx, &obf_func, basic_block.get()); //
                        });
                    }
                }

                /// Apply analysis insn transforms
                if (transform->feature(TransformFeaturesSet::HAS_INSN_TRANSFORM)) {
                    for (auto& basic_block : obf_func.bb_storage->temp_copy()) {
                        for (auto& insn : basic_block->temp_insns_copy()) {
                            execute_transform(tag, [&obf_func, &transform, &insn](auto& ctx) -> void {
                                transform->run_on_insn(ctx, &obf_func, insn.get()); //
                            });
                        }
                    }
                }

                /// Apply program nodes transform
                if (transform->feature(TransformFeaturesSet::HAS_NODE_TRANSFORM)) {
                    for (auto* node = obf_func.program->getHead(); node != nullptr; node = node->getNext()) {
                        /// Transform nodes
                        execute_transform(tag, [&obf_func, &transform, &node](auto& ctx) -> void {
                            transform->run_on_node(ctx, &obf_func, node); //
                        });
                    }
                }

                /// Increment progress bar
                progress.step();
            }

            /// We are done here
        }
    }

    template <pe::any_image_t Img>
    void Instance<Img>::assemble() {
        /// Estimating section size
        auto size_estimation_progress = progress::Progress("obfuscator: estimating section size", functions_.size());
        std::size_t section_size = 0;
        for (auto& func : functions_) {
            const auto program_size = easm::estimate_program_size(*func.analysed.program);
            section_size += memory::address{program_size}.align_up(kTextSectionAlignment).as<std::size_t>();
            size_estimation_progress.step();
        }
        logger::debug("assemble: estimated new section size: {:#x}", section_size);

        /// Allocate new section
        auto img_base = image_->raw_image->get_nt_headers()->optional_header.image_base;
        auto& new_sec = image_->new_section(sections::e_section_t::CODE, section_size);
        memory::address virt_address = new_sec.virtual_address;

        /// Iterate over the obfuscated functions
        auto linking_progress = progress::Progress("obfuscator: linking functions", functions_.size());
        for (auto& [func, _] : functions_) {
            /// \todo @es3n1n: perhaps i should split this monstrosity into a separate functions

            /// Erase the original function code
            for (auto& basic_block : *func.bb_storage) {
                for (auto& insn : basic_block) {
                    /// Clang-tidy is working a bit weird with smart pointers and `bugprone-unchecked-optional-access`
                    auto* raw_ptr = insn.get();

                    /// No need to erase instructions that doesn't exist
                    if (!raw_ptr->rva.has_value() || !raw_ptr->length.has_value()) {
                        continue;
                    }

                    /// Generate random bytes
                    const auto randomized = rnd::bytes(*raw_ptr->length);

                    /// Replace instruction with junk
                    auto* insn_ptr = image_->rva_to_ptr(*raw_ptr->rva);
                    std::memcpy(insn_ptr, randomized.data(), randomized.size());

                    /// Remove pe relocation, if there's any
                    if (raw_ptr->reloc.type == analysis::insn_reloc_t::e_type::HEADER) {
                        image_->relocations.erase(*raw_ptr->rva + raw_ptr->reloc.offset.value_or(0));
                    }
                }
            }

            /// Insert the jmp to obfuscated routine at the very beginning of the function
            auto* func_start_ptr = image_->rva_to_ptr(func.range.start);
            auto jmp_data = easm::encode_jmp(image_->guess_machine_mode(), func.range.start + img_base, virt_address + img_base);
            if (!jmp_data.has_value()) {
                throw std::runtime_error("assemble: unable to encode jmp");
            }
            std::memcpy(func_start_ptr, jmp_data->data(), jmp_data->size());

            /// Assemble the obfuscated function
            auto assemble_progress = progress::Progress(std::format("obfuscator: assembling {}", func.parsed_func.name), 1);
            const auto assembled = easm::assemble_program(virt_address + img_base, *func.program);
            assemble_progress.step();

            /// Copy fresh new assembled function
            std::memcpy( //
                new_sec.raw_data.data() + (virt_address - new_sec.virtual_address).template as<std::size_t>(), //
                assembled.data.data(), //
                assembled.data.size() //
            );

            /// Save the new relocations
            for (const zasm::RelocationInfo& relocation : assembled.relocations) {
                /// Map zasm relocation kind to windows relocation kind
                win::reloc_type_id win_reloc_type; // NOLINT(cppcoreguidelines-init-variables)
                switch (relocation.kind) {
                default:
                case zasm::RelocationType::None:
                    throw std::runtime_error("linker: got invalid relocation");
                case zasm::RelocationType::Abs:
                    win_reloc_type = win::reloc_type_id::rel_based_absolute;
                    break;
                case zasm::RelocationType::Rel32:
                    win_reloc_type = win::reloc_type_id::rel_based_high_low;
                    break;
                }

                /// Store the new relocation data
                image_->relocations[relocation.address - img_base] =
                    pe::relocation_t{.rva = memory::address{static_cast<uintptr_t>(relocation.address - img_base)},
                                     .size = static_cast<std::uint8_t>(getBitSize(relocation.size) / CHAR_BIT),
                                     .type = win_reloc_type};
            }

            /// Align size and increment offset
            const auto aligned_size = memory::address{assembled.data.size()}.align_up(kTextSectionAlignment).as<std::size_t>();
            virt_address = virt_address.offset(aligned_size);

            /// Increment progress bar
            linking_progress.step();
        }

        logger::info("assemble: assembled {} functions", functions_.size());
    }

    template <pe::any_image_t Img>
    std::filesystem::path Instance<Img>::save() {
        logger::info("obfuscator: saving..");
        auto new_img = image_->rebuild_pe_image();

        auto out_path = config_.obfuscator_config().binary_path;

        auto filename = out_path.filename();
        const auto file_ext = filename.extension().string();
        const auto filename_no_ext = filename.replace_extension().string();

        const auto new_filename = filename_no_ext + ".protected" + file_ext;

        out_path = out_path.replace_filename(new_filename);
        files::write_file(out_path, new_img.data(), new_img.size());

        logger::info("obfuscator: saved output to {}", out_path.string());
        return out_path;
    }

    template <pe::any_image_t Img>
    std::filesystem::path Instance<Img>::run() {
        setup();
        obfuscate();
        assemble();
        return save();
    }

    PE_DECL_TEMPLATE_CLASSES(Instance);
} // namespace obfuscator
