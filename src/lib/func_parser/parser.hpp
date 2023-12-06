#pragma once
#include "config_parser/config_parser.hpp"
#include "func_parser/common/common.hpp"
#include "func_parser/common/sanitizer.hpp"

namespace func_parser {
    template <pe::any_image_t Img>
    class Instance {
    public:
        DEFAULT_CTOR(Instance);
        DEFAULT_COPY(Instance);
        ~Instance();

        void setup(Img* image, const config_parser::func_parser_config_t& config, const config_parser::obfuscator_config_t& obfuscator_config) {
            image_ = image;
            config_ = std::move(config);
            obfuscator_config_ = std::move(obfuscator_config);
        }

        void collect_functions();

        std::optional<function_t> find_if(const std::function<bool(const function_t&)>& predicator) {
            const auto iter = std::ranges::find_if(function_list_, predicator);
            if (iter == function_list_.end()) {
                return std::nullopt;
            }

            return std::make_optional<function_t>(*iter);
        }

    private:
        void parse();
        void parse_pdb();
        void parse_map();

        bool push(function_list_t items) {
            if (items.empty()) {
                return false;
            }

            function_lists_.emplace_back(sanitizer::sanitize_function_list(std::move(items), image_));
            return true;
        }

        Img* image_ = nullptr;
        std::vector<function_list_t> function_lists_ = {};
        function_list_t function_list_ = {}; // function_lists_ combined and sanitized basically
        config_parser::func_parser_config_t config_ = {};
        config_parser::obfuscator_config_t obfuscator_config_ = {};
    };
} // namespace func_parser