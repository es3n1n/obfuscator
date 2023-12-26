#pragma once
#include "util/logger.hpp"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <vector>

#include "reader.hpp"

// \fixme: @es3n1n: this file is a mess
// \todo: @es3n1n: add += -= operators
// \todo: @es3n1n: add self_* funcs

namespace memory {
    class address {
    public:
        constexpr address() = default;

        /// Implicit conversions ftw
        constexpr address(const std::nullptr_t) { } // NOLINT
        constexpr address(const uintptr_t address): address_(address) { } // NOLINT
        address(const void* address): address_(reinterpret_cast<uintptr_t>(address)) { } // NOLINT
        address(const std::vector<std::uint8_t>& data): address_(reinterpret_cast<uintptr_t>(data.data())) { } // NOLINT

        address(const address& inst) = default;
        address(address&& inst) = default;
        address& operator=(const address& inst) = default;
        address& operator=(address&& inst) = default;
        ~address() = default;

        [[nodiscard]] constexpr address offset(const std::ptrdiff_t offset = 0) const noexcept {
            if (address_ == 0U) {
                return *this;
            }

            return {address_ + offset};
        }

        std::expected<address, e_error_code> write(const void* buffer, const std::size_t size) {
            if (auto result = memory::write(address_, buffer, size); !result.has_value()) {
                return std::unexpected(result.error());
            }

            return *this;
        }

        template <typename Ty>
        std::expected<address, e_error_code> write(Ty value) {
            auto copy = std::move(value);

            if (auto ret = memory::write(&copy, address_); !ret.has_value()) {
                return std::unexpected(ret.error());
            }

            return *this;
        }

        template <typename Ty>
        [[nodiscard]] std::expected<Ty, e_error_code> read() const {
            return memory::read<Ty>(address_);
        }

        template <typename Ty>
        std::expected<Ty*, e_error_code> read(Ty* dst) const {
            if (auto result = memory::read(dst, address_); !result.has_value()) {
                return std::unexpected(result.error());
            }

            return dst;
        }

        [[nodiscard]] std::expected<std::vector<std::uint8_t>, e_error_code> read_vector(const std::size_t size) const {
            std::vector<std::uint8_t> result = {};
            result.resize(size);

            if (auto stat = memory::read(result.data(), address_, size); !stat.has_value()) {
                return std::unexpected(stat.error());
            }

            return result;
        }

        template <typename T = address>
        [[nodiscard]] std::expected<T, e_error_code> deref() const {
            auto result = memory::read<T>(inner());

            if (!result.has_value()) {
                return std::unexpected(result.error());
            }

            return result.value();
        }

        template <typename T = address>
        [[nodiscard]] std::expected<T, e_error_code> get(std::size_t count = 1) const noexcept {
            if (!address_ || count == 0) {
                return std::unexpected(e_error_code::INVALID_ADDRESS);
            }

            address _tmp = *this;
            while (_tmp && count-- >= 2) {
                auto deref_value = _tmp.deref();
                if (!deref_value.has_value()) {
                    return std::unexpected(e_error_code::NOT_ENOUGH_BYTES);
                }

                _tmp = deref_value.value();
            }

            return _tmp.deref<T>();
        }

        template <typename T = address>
        [[nodiscard]] constexpr T* ptr(const std::ptrdiff_t offset = 0) const noexcept {
            return this->offset(offset).as<std::add_pointer_t<T>>();
        }

        template <typename T = address>
        [[nodiscard]] constexpr T* self_inc_ptr(const std::ptrdiff_t offset = 0) noexcept {
            auto* result = ptr<T>(offset);
            *this = address{result}.offset(sizeof(T));
            return result;
        }

        template <typename Ty>
        std::expected<address, e_error_code> self_write_inc(const Ty data, const std::ptrdiff_t offset = 0) noexcept {
            auto result = this->offset(offset).write(data);
            *this = this->offset(offset + sizeof(Ty));
            return result;
        }

        [[nodiscard]] constexpr address align_down(const std::ptrdiff_t factor) const noexcept {
            return {address_ & ~(factor - 1U)};
        }

        [[nodiscard]] constexpr address align_up(const std::ptrdiff_t factor) const noexcept {
            return address{address_ + factor - 1U}.align_down(factor);
        }

        template <typename T>
        [[nodiscard]] constexpr T cast() const noexcept {
            return memory::cast<T>(address_);
        }

        template <typename T>
        [[nodiscard]] constexpr T as() const noexcept {
            return cast<T>();
        }

        constexpr explicit operator std::uintptr_t() const noexcept {
            return address_;
        }

        [[nodiscard]] constexpr std::uintptr_t inner() const noexcept {
            return address_;
        }

        constexpr explicit operator bool() const noexcept {
            return static_cast<bool>(address_);
        }

#define MATH_OPERATOR(type, operation)                             \
    constexpr type operator operation(const address& rhs) const {  \
        return static_cast<type>(address_ operation rhs.address_); \
    }

        MATH_OPERATOR(bool, ==)
        MATH_OPERATOR(bool, !=)
        MATH_OPERATOR(bool, >)
        MATH_OPERATOR(bool, <)
        MATH_OPERATOR(bool, <=)
        MATH_OPERATOR(bool, >=)
        MATH_OPERATOR(address, +)
        MATH_OPERATOR(address, -)

#undef MATH_OPERATOR

    private:
        std::uintptr_t address_ = 0;
    };
} // namespace memory

//
// Creating custom formatters for the std::format function so that
// we can easily format addresses in logger
//
template <>
struct std::formatter<memory::address> : std::formatter<std::uintptr_t> {
    template <class FormatContextTy>
    constexpr auto format(const memory::address& instance, FormatContextTy& ctx) const {
        return std::formatter<std::uintptr_t>::format(instance.inner(), ctx);
    }
};

//
// Overriding custom implementation for std::hash in order to use
// this type in containers
//
template <>
struct std::hash<memory::address> {
    size_t operator()(const memory::address& instance) const noexcept {
        return std::hash<std::uintptr_t>()(instance.inner());
    }
};
