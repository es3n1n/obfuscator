#pragma once
#include "pe/pe.hpp"
#include <variant>

// there are no other ways, it seems.
// NOLINTNEXTLINE
#define UNWRAP_IMAGE(ty, fn) detail::visit_wrapped_image<ty>(image, &fn<pe::Image<win::image_x64_t>>, &fn<pe::Image<win::image_x86_t>>, data)

namespace pe::detail {
    template <typename T>
    struct BasePtrWrapper {
        T* ptr;
    };

    struct X64PtrWrapper : BasePtrWrapper<Image<win::image_x64_t>> { };
    struct X86PtrWrapper : BasePtrWrapper<Image<win::image_x86_t>> { };

    using ImgWrapped = std::variant<X64PtrWrapper, X86PtrWrapper>;

    template <any_image_t Ty>
    ImgWrapped wrap_image(Ty* image) {
        using WrapperTy = std::conditional_t<std::is_same_v<Ty, Image<win::image_x64_t>>, X64PtrWrapper, X86PtrWrapper>;
        return ImgWrapped(WrapperTy{image});
    }

    template <typename RetTy = void>
    RetTy visit_wrapped_image(ImgWrapped image_wrapped, //
                              std::function<RetTy(Image<win::image_x64_t>*, std::vector<std::uint8_t>&)> x64_visitor,
                              std::function<RetTy(Image<win::image_x86_t>*, std::vector<std::uint8_t>&)> x86_visitor, //
                              std::vector<std::uint8_t>& data) {
        return std::visit<RetTy>(
            [&](auto&& inst) -> RetTy {
                if constexpr (std::is_same_v<std::decay_t<decltype(inst)>, X64PtrWrapper>) {
                    return x64_visitor(inst.ptr, data);
                } else {
                    return x86_visitor(inst.ptr, data);
                }
            },
            image_wrapped);
    }

    template <typename Ty>
    [[nodiscard]] Ty* buffer_pointer(std::vector<std::uint8_t>& data) {
        return memory::cast<Ty*>(data.data());
    }
} // namespace pe::detail
