#include "config_parser/config_parser.hpp"
#include "cont/cont.hpp"
#include "obfuscator/obfuscator.hpp"
#include "obfuscator/transforms/scheduler.hpp"
#include <es3n1n/common/files.hpp>
#include <es3n1n/common/logger.hpp>
#include <es3n1n/common/random.hpp>

namespace {
    int startup(config_parser::Config& config) {
        rnd::detail::seed(config.obfuscator_config().seed);
        const auto& binary_path = config.obfuscator_config().binary_path;

        logger::info("main: loading binary from {}", binary_path.string());
        auto file = files::read_file(binary_path);
        if (!file.has_value() || file->empty()) {
            throw std::runtime_error("Got empty binary");
        }

        std::unique_ptr<cont::ImageBase> image;
        switch (cont::get_image_type(*file)) {
        case cont::ContImageType::PE:
            image = std::make_unique<cont::pe::Image>(file->data());
            logger::info("main: PE image loaded");
            break;
        case cont::ContImageType::ELF:
            image = std::make_unique<cont::elf::Image>(file->data());
            logger::info("main: ELF image loaded");
            break;
        default:
            throw std::runtime_error("Got unsupported image type");
        }

        obfuscator::Instance inst(image.get(), config);
        inst.run();

        logger::info("startup: bye-bye");
        return 0;
    }
} // namespace

int main(const int argc, const char* argv[]) try {
    obfuscator::startup_scheduler();

    auto config = config_parser::from_argv(argc, argv);
    return startup(config);
} catch (std::exception& err) {
    logger::critical("RUNTIME ERROR: {}", err.what());
    return 1;
} catch (...) {
    logger::critical("Unknown runtime error");
    return 1;
}
