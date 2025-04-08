#include "config_parser/config_parser.hpp"
#include "cli/cli.hpp"
#include "obfuscator/transforms/configs.hpp"

namespace config_parser {
    Config from_argv(std::size_t argc, const char* argv[]) {
        /// No binary path
        if (argc < 2) {
            cli::print_help(argv);
        }

        /// Get the binary path, check for some meme stuff
        const auto binary_path = std::string_view{argv[1]};
        if (binary_path == "-h" || binary_path == "--help") {
            cli::print_help(argv);
        }

        /// Allocate result
        Config result = {};
        auto& [binary_path_value, seed] = result.obfuscator_config();
        auto& func_parser_config = result.func_parser_config();

        /// Get some stuff for transforms resolving
        auto& shared_config_storage = obfuscator::TransformSharedConfigStorage::get();

        /// Save the binary path
        binary_path_value = binary_path;

        /// Some state stuff for parser
        struct {
            function_configuration_t* current_function = nullptr;
            transform_configuration_t* current_transform = nullptr;

            [[nodiscard]] bool busy() const {
                return static_cast<bool>(current_function);
            }
        } state;

        for (std::size_t i = 2; i < argc; ++i) {
            /// Get current arguments
            const std::string arg_ = argv[i];

            /// \todo @es3n1n: Create a wrapper for these cringe checks
            std::optional<std::string> next_arg_ = i + 1 < argc && argv[i + 1][0] != '-' ? std::make_optional<std::string>(argv[i + 1]) : std::nullopt;
            std::optional<std::string> next_next_arg_ =
                i + 2 < argc && argv[i + 2][0] != '-' ? std::make_optional<std::string>(argv[i + 2]) : std::nullopt;

            /// Util to skip arguments, if needed
            auto skip = [&](const std::size_t count) -> void {
                if (!next_arg_.has_value()) {
                    return;
                }

                i += 1;
                if (count < 2) {
                    return;
                }

                if (!next_next_arg_.has_value()) {
                    return;
                }
                i += 1;
            };

            /// Help
            if (arg_ == "-h" || arg_ == "--help" || arg_ == "--version") {
                cli::print_help(argv);
                continue;
            }

            /// PDB path
            if (arg_ == "-pdb") {
                func_parser_config.pdb_enabled = true;
                func_parser_config.pdb_path = next_arg_;
                skip(1);
                continue;
            }

            /// Map path
            if (arg_ == "-map") {
                func_parser_config.map_enabled = true;
                func_parser_config.map_path = next_arg_;
                skip(1);
                continue;
            }

            /// Function start
            if (arg_ == "-f" && next_arg_.has_value()) {
                state.current_function = &result.create_function_config();
                state.current_function->function_name = next_arg_.value();
                skip(1);
                continue;
            }

            /// Transform configuration start
            if (arg_ == "-t" && next_arg_.has_value() && state.busy()) {
                state.current_transform = &state.current_function->transform_configurations.emplace_back();
                state.current_transform->tag = shared_config_storage.get_for_name(next_arg_.value()).tag;
                skip(1);
                continue;
            }

            /// Transform global configuration
            if (arg_ == "-g" && next_arg_.has_value()) {
                state.current_transform = &result.create_global_transform_config();
                state.current_transform->tag = shared_config_storage.get_for_name(next_arg_.value()).tag;
                skip(1);
                continue;
            }

            /// Push value
            if (arg_ == "-v" && next_arg_.has_value() && next_next_arg_.has_value() && state.current_transform != nullptr) {
                state.current_transform->values[next_arg_.value()] = next_next_arg_.value();
                skip(2);
            }

            if (arg_ == "-seed" && next_arg_.has_value()) {
                /// \todo @sovissa:
                /// [1] add correct processing of literals to string_parser
                /// [2] add support for negative values to arg parser
                std::size_t seed_value_base = 10;

                std::string_view next_arg_view{next_arg_.value()};
                if (next_arg_view.starts_with('-')) {
                    next_arg_view = next_arg_view.substr(1);
                }
                if (next_arg_view.length() >= 2 && //
                    (next_arg_view.starts_with("0x") || next_arg_view.starts_with("0X"))) {
                    seed_value_base = 16;
                }

                seed = string_parser::parse_uint64(next_arg_.value(), seed_value_base);
                skip(1);
                continue;
            }
        }

        return result;
    }
} // namespace config_parser
