#pragma once
#include "analysis/analysis.hpp"
#include "cont/base.hpp"

namespace analysis {
    struct PassContext {
        cont::ImageMode image_mode;
        std::optional<cont::ImageBase*> image;
        Function* function;
    };
} // namespace analysis