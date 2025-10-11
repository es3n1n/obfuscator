#pragma once
#include "analysis/analysis.hpp"
#include "config_parser/config_parser.hpp"
#include "func_parser/parser.hpp"
#include "obfuscator/function.hpp"
#include "util/structs.hpp"

namespace obfuscator {
    class Instance {
    public:
        Instance(cont::ImageBase* image, config_parser::Config& config): image_(image), image_mode_(image->mode()), config_(std::move(config)) { }
        explicit Instance(const cont::ImageMode image_mode): image_(std::nullopt), image_mode_(image_mode) { }

        DEFAULT_DTOR(Instance);
        NON_COPYABLE(Instance);

        struct function_t {
            analysis::Function analysed;
            config_parser::function_configuration_t configuration;
        };

        struct nameless_function_t {
            analysis::Function analysed;
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
        void obfuscate(const config_parser::transform_configurations_t& configurations, Function& function, const std::string& function_name) const;

        /// \fixme @es3n1n: this is wrong
        [[nodiscard]] zasm::MachineMode machine_mode() const {
            return image_mode_ == cont::ImageMode::X64 ? zasm::MachineMode::AMD64 : zasm::MachineMode::I386;
        }

        std::optional<cont::ImageBase*> image_ = nullptr;
        cont::ImageMode image_mode_;
        config_parser::Config config_;
        func_parser::Instance func_parser_;
        std::vector<function_t> functions_;
        std::vector<nameless_function_t> nameless_functions_;
    };
} // namespace obfuscator