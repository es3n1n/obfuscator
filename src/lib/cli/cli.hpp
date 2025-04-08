#pragma once
#include "obfuscator/transforms/configs.hpp"
#include "obfuscator/transforms/scheduler.hpp"
#include <es3n1n/common/logger.hpp>

namespace cli {
    namespace detail {
        /// { { name, arg1, arg2 }, description }
        constexpr auto kCLIOptionsHelp = std::to_array<std::pair<std::array<std::string_view, 3>, std::string_view>>({
            {{"-h, --help", "", ""}, "This message"},
            {{"-pdb", "[path]", ""}, "Set custom .pdb file location"},
            {{"-map", "[path]", ""}, "Set custom .map file location"},
            {{"-f", "[name]", ""}, "Start new function configuration"},
            {{"-t", "[name]", ""}, "Start new transform configuration"},
            {{"-g", "[name]", ""}, "Start new transform global configuration"},
            {{"-v", "[name]", "[value]"}, "Push value"},
        });

        template <pe::any_image_t Img>
        void dump_transforms(const std::string_view platform_name) {
            /// Header
            logger::info("Available {} transforms:", platform_name);

            /// Iterate over the transforms
            for (auto& scheduler = obfuscator::TransformScheduler::get().for_arch<Img>(); //
                 auto& [tag, transform] : scheduler.transforms) {
                /// Get the shared cfg
                auto& shared_cfg = obfuscator::TransformSharedConfigStorage::get().get_for(tag);

                /// Dump transform name
                logger::info<1>("{}", shared_cfg.name);

                /// Dump variables
                std::once_flag fl;
                transform->iter_vars([&fl](const obfuscator::TransformConfig::Var& var) -> void {
                    std::call_once(fl, []() -> void { logger::info<2>("Variables:"); });

                    logger::info<3>("{}", var.name());
                    logger::info<4>("type: {}", var.var_type() == obfuscator::TransformConfig::Var::Type::GLOBAL ? "global" : "per function");
                    logger::info<4>("default: {}", var.serialize());
                    logger::info<4>("required: {}", var.required() ? "true" : "false");
                    if (const auto short_desc = var.short_description(); short_desc.has_value()) {
                        logger::info<4>("description: {}", short_desc.value());
                    }
                });
            }
        }

        inline void dump_shared_vars() {
            /// Banner
            logger::info("Shared transform variables (e.g could be set for every transform):");

            /// Get some scheduler and any transform + shared config for it
            const auto& scheduler = obfuscator::TransformScheduler::get().for_arch<pe::X64Image>();
            const auto& shared_cfg = obfuscator::TransformSharedConfigStorage::get().get_for(scheduler.transforms.begin()->first);

            /// Dump all vars + their defaults
            for (const auto& name : obfuscator::detail::kSharedConfigsVariableNames) {
                logger::info<1>("{:<12} -- default: {}", name, shared_cfg.stringify_var(name));
            }
        }
    } // namespace detail

    inline void print_help(const char* argv[]) {
        logger::enabled = true; // just to be sure
        logger::info("github.com/es3n1n/obfuscator - A PoC native code obfuscator");
        auto pad = [] {
            logger::info(" ");
        };
        pad();

        logger::info("Usage: {} [binary] [options...]", argv[0]);
        pad();

        logger::info("Available options:");
        for (const auto& [args, desc] : detail::kCLIOptionsHelp) {
            logger::info<1>("{:<12} {:<6} {:<8} -- {}", args[0], args[1], args[2], desc);
        }
        pad();

        logger::info("Examples:");
        logger::info<1>("obfuscator hehe.exe -f main -t TransformName -v SomeName 1337");
        logger::info<1>("obfuscator hehe.exe -f main -t TransformName -v SomeName 1337 -g TransformName -v SomeGlobalName 1337");
        logger::info<1>("obfuscator hehe.exe -f main -t TransformName -v SomeName 1337 -v SomeName0 1337 -g TransformName -v SomeGlobalName 1337");
        logger::info<1>("obfuscator hehe.exe -map mymap.map -pdb mypdb.pdb -f main -t TransformName -v SomeName 1337 -v SomeName0 1337 -g TransformName "
                        "-v SomeGlobalName 1337");
        pad();

        detail::dump_transforms<pe::X64Image>("x64");
        pad();

        detail::dump_transforms<pe::X86Image>("x86");
        pad();

        detail::dump_shared_vars();
        pad();

        std::exit(0);
    }
} // namespace cli