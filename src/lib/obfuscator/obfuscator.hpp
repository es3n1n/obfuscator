#pragma once
#include "analysis/analysis.hpp"
#include "config_parser/config_parser.hpp"
#include "func_parser/parser.hpp"
#include "obfuscator/function.hpp"
#include "pe/pe.hpp"
#include "util/structs.hpp"

namespace obfuscator {
    template <pe::any_image_t Img>
    class Instance {
    public:
        Instance(Img* image, config_parser::Config& config): image_(image), config_(std::move(config)) { }
        Instance(): image_(std::nullopt), config_({}) { }

        DEFAULT_DTOR(Instance);
        NON_COPYABLE(Instance);

        struct function_t {
            analysis::Function<Img> analysed;
            config_parser::function_configuration_t configuration;
        };

        struct nameless_function_t {
            analysis::Function<Img> analysed;
            config_parser::nameless_function_configuration_t configuration;
        };

        /// Parses functions using `func_parser_`, enables transforms in the scheduler
        void setup();

        /// Adds a function to obfuscate from its raw binary representation (mostly used by tests)
        nameless_function_t& add_function(std::span<std::uint8_t> raw_function_bytes,
                                          const config_parser::nameless_function_configuration_t& configuration);
        /// Adds a function to obfuscate by name (will be looked up in `func_parser_`)
        function_t& add_function(const config_parser::function_configuration_t& configuration);

        /// Run over the added functions and apply enabled transforms
        void obfuscate();

        /// 1. Allocates new section for our code
        /// 2. Erases original function's code/relocations
        /// 3. Links obfuscated functions
        /// 4. Writes the obfuscated functions to the new section
        /// 4. Saves new relocations
        void assemble();

        /// Rebuilds PE with new data we got and saves it to disk
        std::filesystem::path save();

        /// setup() -> obfuscate() -> assemble() -> save()
        std::filesystem::path run();

    private:
        static void schedule_transforms(const config_parser::transform_configurations_t& configurations);
        void obfuscate(const config_parser::transform_configurations_t& configurations, Function<Img>& function, const std::optional<std::string>& function_name = std::nullopt);

        std::optional<Img*> image_ = nullptr;
        config_parser::Config config_;
        func_parser::Instance<Img> func_parser_;
        std::vector<function_t> functions_;
        std::vector<nameless_function_t> nameless_functions_;
    };
} // namespace obfuscator