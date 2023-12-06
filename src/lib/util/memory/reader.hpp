#pragma once
#include "casts.hpp"
#include <cstdint>
#include <expected>

namespace memory {
    enum class e_error_code : std::uint8_t {
        INVALID_PARAMETERS = 0,
        INVALID_ADDRESS,
        NOT_ENOUGH_BYTES
    };

    inline std::expected<std::size_t, e_error_code> read(void* buffer, const std::uintptr_t address, const std::size_t size) {
        if (buffer == nullptr || address == 0U || size == 0U) {
            return std::unexpected(e_error_code::INVALID_PARAMETERS);
        }

        auto trivial_copy = [&buffer, &address]<typename T>(T unused_var [[maybe_unused]]) -> void {
            *static_cast<T*>(buffer) = *reinterpret_cast<T*>(address);
        };

        switch (size) {
        case sizeof(uint8_t):
            trivial_copy(uint8_t{});
            break;

        case sizeof(uint16_t):
            trivial_copy(uint16_t{});
            break;

        case sizeof(uint32_t):
            trivial_copy(uint32_t{});
            break;

        case sizeof(uint64_t):
            trivial_copy(uint64_t{});
            break;

        default:
            // NOLINTNEXTLINE
            std::memcpy(buffer, reinterpret_cast<void*>(address), size);
            break;
        }

        return size;
    }

    inline std::expected<std::size_t, e_error_code> write(std::uintptr_t address, const void* buffer, const std::size_t size) {
        if (buffer == nullptr || address == 0U || size == 0U) {
            return std::unexpected(e_error_code::INVALID_PARAMETERS);
        }

        auto trivial_copy = [&buffer, &address]<typename T>(T unused_var [[maybe_unused]]) -> void {
            // NOLINTNEXTLINE
            *reinterpret_cast<T*>(address) = *static_cast<T*>(const_cast<void*>(buffer));
        };

        switch (size) {
        case sizeof(uint8_t):
            trivial_copy(uint8_t{});
            break;

        case sizeof(uint16_t):
            trivial_copy(uint16_t{});
            break;

        case sizeof(uint32_t):
            trivial_copy(uint32_t{});
            break;

        case sizeof(uint64_t):
            trivial_copy(uint64_t{});
            break;

        default:
            // NOLINTNEXTLINE
            std::memcpy(reinterpret_cast<void*>(address), buffer, size);
            break;
        }

        return size;
    }

    template <typename Ty>
    std::expected<std::size_t, e_error_code> read(Ty* dst, const std::uintptr_t src) {
        return read(dst, src, sizeof(Ty));
    }

    template <typename Ty>
    std::expected<Ty, e_error_code> read(const std::uintptr_t src) {
        Ty _obj = {};

        if (const auto res = read(&_obj, src); !res.has_value()) {
            return std::unexpected(res.error());
        }

        return _obj;
    }

    template <typename Ty>
    std::expected<std::size_t, e_error_code> write(const Ty* src, const std::uintptr_t dst) {
        return write(memory::cast<const uintptr_t>(dst), src, sizeof(Ty));
    }
} // namespace memory