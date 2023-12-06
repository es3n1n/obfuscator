#pragma once
#include "analysis/analysis.hpp"
#include "easm/cursor/cursor.hpp"
#include "func_parser/parser.hpp"
#include "pe/pe.hpp"
#include "util/types.hpp"

namespace obfuscator {
    /// \brief Transform tags
    using TransformTag = std::size_t;

    namespace transforms {
        /// \brief Dummy class for junk detection
        template <pe::any_image_t>
        class DummyTransform { };
    } // namespace transforms

    namespace detail {
        /// \brief Raw funcsig getter
        template <template <pe::any_image_t> class C> // C is not unused!
        [[nodiscard]] constexpr std::string_view get_funcsig() {
#if PLATFORM_IS_MSVC
            return __FUNCSIG__;
#else
            if constexpr (sizeof(__PRETTY_FUNCTION__) == sizeof(__FUNCTION__)) {
                static_assert(types::always_false_v<>, "Unsupported pretty function");
            }
            return __PRETTY_FUNCTION__;
#endif
        }

        /// \brief Junk info storage
        struct junk_info_t {
            std::size_t start;
            std::size_t total;
        };

        /// \brief Junk info
        constexpr junk_info_t junk_info = []() -> junk_info_t {
            constexpr std::string_view symbol = "DummyTransform";
            std::string_view sample = get_funcsig<transforms::DummyTransform>();
            return junk_info_t{.start = sample.find(symbol), //
                               .total = sample.size() - symbol.size()};
        }();
        static_assert(junk_info.start != static_cast<std::size_t>(-1), "Unable to find junk");

        /// \brief Transform name getter as an array
        template <template <pe::any_image_t> class C>
        constexpr static auto transform_name = [] {
            constexpr auto funcsig = get_funcsig<C>();
            std::array<char, funcsig.size() - junk_info.total + 1> ret{};
            std::copy_n(funcsig.data() + junk_info.start, ret.size() - 1, ret.data());
            return ret;
        }();
    } // namespace detail

    /// \brief Transform name getter
    template <template <pe::any_image_t> class C>
    [[nodiscard]] constexpr std::string_view get_transform_name() noexcept {
        return {detail::transform_name<C>.data(), detail::transform_name<C>.size() - 1};
    }

    /// \brief Transform tag getter
    template <template <pe::any_image_t> class C>
    [[nodiscard]] constexpr TransformTag get_transform_tag() noexcept {
        constexpr std::hash<std::string_view> hash;
        return hash(get_transform_name<C>());
    }
} // namespace obfuscator