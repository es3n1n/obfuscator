#pragma once
#include "analysis/analysis.hpp"
#include "config_parser/config_parser.hpp"
#include "func_parser/parser.hpp"
#include "pe/pe.hpp"
#include "util/structs.hpp"

namespace obfuscator {
    template <pe::any_image_t Img>
    class Instance {
    public:
        Instance(Img* image, config_parser::Config& config): image_(image), config_(std::move(config)) { }
        DEFAULT_DTOR(Instance);
        NON_COPYABLE(Instance);

        void setup();
        void add_function(const config_parser::function_configuration_t& configuration);
        void obfuscate();
        void assemble();
        void save();

        struct function_t {
            analysis::Function<Img> analysed;
            config_parser::function_configuration_t configuration;
        };

    private:
        Img* image_ = nullptr;
        config_parser::Config config_ = {};
        func_parser::Instance<Img> func_parser_ = {};
        std::vector<function_t> functions_ = {};
    };
} // namespace obfuscator