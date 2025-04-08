#pragma once
#include "config_parser/config_parser.hpp"
#include "func_parser/common/common.hpp"
#include "func_parser/common/demangler.hpp"
#include "func_parser/common/sanitizer.hpp"
#include <es3n1n/common/progress.hpp>

namespace func_parser {
    template <pe::any_image_t Img>
    class Instance {
    public:
        DEFAULT_CT_CTOR_DTOR(Instance);
        DEFAULT_COPY(Instance);

        void setup(Img* image, const config_parser::func_parser_config_t& config, const config_parser::obfuscator_config_t& obfuscator_config) {
            image_ = image;
            config_ = config;
            obfuscator_config_ = obfuscator_config;
            progress_.emplace("func_parser: discovering functions", 4);
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

            sanitizer::sanitize_function_list(items, image_);
            demangler::demangle_functions(items);

            function_lists_.emplace_back(items);
            return true;
        }

        void progress_step() {
            if (!progress_.has_value()) {
                return;
            }

            progress_->step();
        }

        Img* image_ = nullptr;
        std::vector<function_list_t> function_lists_;
        function_list_t function_list_; // function_lists_ combined and sanitized basically
        config_parser::func_parser_config_t config_ = {};
        config_parser::obfuscator_config_t obfuscator_config_ = {};
        std::optional<progress::Progress> progress_ = std::nullopt;
    };
} // namespace func_parser