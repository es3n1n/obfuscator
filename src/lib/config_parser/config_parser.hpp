#pragma once
#include "config_parser/structs.hpp"
#include "util/structs.hpp"

#include <vector>

namespace config_parser {
    class Config {
    public:
        DEFAULT_CTOR_DTOR(Config);
        DEFAULT_COPY(Config);

        [[nodiscard]] function_configuration_t& create_function_config() {
            return function_configurations_.emplace_back();
        }

        [[nodiscard]] transform_configuration_t& create_global_transform_config() {
            return global_transform_configurations_.emplace_back();
        }

        [[nodiscard]] obfuscator_config_t& obfuscator_config() {
            return obfuscator_config_;
        }

        [[nodiscard]] func_parser_config_t& func_parser_config() {
            return func_parser_config_;
        }

        [[nodiscard]] std::vector<transform_configuration_t> global_transforms_config() {
            return global_transform_configurations_;
        }

        [[nodiscard]] auto begin() {
            return function_configurations_.begin();
        }

        [[nodiscard]] auto begin() const {
            return function_configurations_.begin();
        }

        [[nodiscard]] auto end() {
            return function_configurations_.end();
        }

        [[nodiscard]] auto end() const {
            return function_configurations_.end();
        }

    private:
        std::vector<function_configuration_t> function_configurations_ = {};
        std::vector<transform_configuration_t> global_transform_configurations_ = {};
        obfuscator_config_t obfuscator_config_ = {};
        func_parser_config_t func_parser_config_ = {};
    };

    Config from_argv(std::size_t argc, char* argv[]);
} // namespace config_parser
