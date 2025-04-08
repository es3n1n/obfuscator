#include "config_parser/config_parser.hpp"
#include "obfuscator/obfuscator.hpp"
#include "obfuscator/transforms/scheduler.hpp"
#include "pe/arch/arch.hpp"
#include "pe/common/common.hpp"
#include <es3n1n/common/files.hpp>
#include <es3n1n/common/logger.hpp>
#include <es3n1n/common/random.hpp>

namespace {
    template <pe::any_raw_image_t Img>
    void bootstrap(Img* raw_image, config_parser::Config& config) {
        pe::Image<Img> image(raw_image);

        obfuscator::Instance<decltype(image)> inst(&image, config);
        inst.run();

        logger::info("startup: bye-bye");
    }

    int startup(config_parser::Config& config) try {
        rnd::detail::seed(config.obfuscator_config().seed);
        const auto& binary_path = config.obfuscator_config().binary_path;

        logger::info("main: loading binary from {}", binary_path.string());
        auto file = files::read_file(binary_path);
        if (!file.has_value() || file->empty()) {
            throw std::runtime_error("Got empty binary");
        }

        auto* img_x64 = reinterpret_cast<win::image_x64_t*>(file->data());
        auto* img_x86 = reinterpret_cast<win::image_x86_t*>(img_x64);

        if (!pe::common::is_valid(img_x64)) {
            throw std::runtime_error("Invalid pe header");
        }

        if (pe::arch::is_x64(img_x64)) {
            bootstrap(img_x64, config);
        } else {
            bootstrap(img_x86, config);
        }

        return 0;
    } catch (std::runtime_error& err) {
        logger::critical("RUNTIME ERROR: {}", err.what());
        return 1;
    }
} // namespace

int main(const int argc, const char* argv[]) try {
    obfuscator::startup_scheduler();

    auto config = config_parser::from_argv(argc, argv);
    return startup(config);
} catch (...) {
    logger::critical("Unknown runtime error");
    return 1;
}
