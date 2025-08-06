#pragma once
#include "analysis/analysis.hpp"
#include "pe/pe.hpp"

namespace analysis {
    template <pe::any_image_t Img>
    struct PassContext {
        std::optional<Img*> image;
        Function<Img>* function;
    };
} // namespace analysis