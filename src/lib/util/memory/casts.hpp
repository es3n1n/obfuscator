#pragma once

namespace memory {
    template <typename DstTy, typename SrcTy>
    DstTy cast(SrcTy src) {
        if constexpr (std::is_same_v<std::remove_cv_t<DstTy>, decltype(src)>) {
            return src;
        } else if constexpr (sizeof(DstTy) < sizeof(src)) {
            return static_cast<DstTy>(src);
        } else {
            // NOLINTNEXTLINE
            return (DstTy)src;
        }
    }
} // namespace memory
