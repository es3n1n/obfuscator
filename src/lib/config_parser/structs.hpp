#pragma once
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

namespace config_parser {
    struct transform_configuration_t {
        std::size_t tag = {};
        std::unordered_map<std::string, std::string> values;
    };

    using transform_configurations_t = std::vector<transform_configuration_t>;

    struct nameless_function_configuration_t {
        transform_configurations_t transform_configurations;
    };

    struct function_configuration_t {
        std::optional<std::string> function_name;
        std::optional<std::ptrdiff_t> rva;
        transform_configurations_t transform_configurations;

        [[nodiscard]] std::string name() const {
            if (function_name.has_value()) {
                return *function_name;
            }
            if (rva.has_value()) {
                return std::format("sub_{:x}", *rva);
            }
            /// \todo @es3n1n: Swap nameless funcs to use this struct too
            return "nameless";
        }
    };

    struct obfuscator_config_t {
        std::filesystem::path binary_path = "";
        std::optional<std::uint64_t> seed = std::nullopt;
    };

    struct func_parser_config_t {
        bool pdb_enabled = true;
        std::optional<std::filesystem::path> pdb_path = std::nullopt;

        bool map_enabled = false;
        std::optional<std::filesystem::path> map_path = std::nullopt;
    };
} // namespace config_parser