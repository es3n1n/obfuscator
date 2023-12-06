#pragma once
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

namespace config_parser {
    struct transform_configuration_t {
        std::size_t tag = {};
        std::unordered_map<std::string, std::string> values = {};
    };

    struct function_configuration_t {
        std::string function_name = "";
        std::vector<transform_configuration_t> transform_configurations = {};
    };

    struct obfuscator_config_t {
        std::filesystem::path binary_path = "";
    };

    struct func_parser_config_t {
        bool pdb_enabled = true;
        std::optional<std::filesystem::path> pdb_path = std::nullopt;

        bool map_enabled = false;
        std::optional<std::filesystem::path> map_path = std::nullopt;
    };
} // namespace config_parser