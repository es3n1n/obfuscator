#pragma once
#include "types.hpp"

#include <any>
#include <cassert>
#include <cstdint>
#include <string>

namespace util::string {
    /// \brief Parse int32 from string
    /// \param s string that contain int32
    /// \param base base (10 for decimal, 16 for hex, etc)
    /// \return parsed value
    [[nodiscard]] inline std::int32_t parse_int32(const std::string_view s, const std::size_t base = 10) {
        return std::stol(s.data(), nullptr, static_cast<int>(base));
    }

    /// \brief Parse uint32 from string
    /// \param s string that contain uint32
    /// \param base base (10 for decimal, 16 for hex, etc)
    /// \return parsed value
    [[nodiscard]] inline std::uint32_t parse_uint32(const std::string_view s, const std::size_t base = 10) {
        return std::stoul(s.data(), nullptr, static_cast<int>(base));
    }

    /// \brief Parse int8 from string
    /// \param s string that contain int8
    /// \param base base (10 for decimal, 16 for hex, etc)
    /// \return parsed value
    [[nodiscard]] inline std::int8_t parse_int8(const std::string_view s, const std::size_t base = 10) {
        return parse_int32(s, base) & 0xFF;
    }

    /// \brief Parse uint8 from string
    /// \param s string that contain uint8
    /// \param base base (10 for decimal, 16 for hex, etc)
    /// \return parsed value
    [[nodiscard]] inline std::uint8_t parse_uint8(const std::string_view s, const std::size_t base = 10) {
        return parse_uint32(s, base) & 0xFF;
    }

    /// \brief Parse bool from string
    /// \param s stirng that contain bool
    /// \return parsed value
    [[nodiscard]] inline bool parse_bool(const std::string_view s) {
        return s == "true" || s == "1";
    }

    namespace detail {
        template <typename Ty, typename TyNoCV = std::remove_cv_t<Ty>>
        struct serializer_ctx {
            constexpr static bool is_int32 = std::is_same_v<TyNoCV, int> || std::is_same_v<TyNoCV, long>;
            constexpr static bool is_uint32 = std::is_same_v<TyNoCV, unsigned int> || std::is_same_v<TyNoCV, unsigned long>;
            constexpr static bool is_int8 = std::is_same_v<TyNoCV, char>;
            constexpr static bool is_uint8 = std::is_same_v<TyNoCV, unsigned char>;

            constexpr static bool is_number = is_int32 || is_uint32 || is_int8 || is_uint8;
            constexpr static bool is_bool = std::is_same_v<TyNoCV, bool>;
        };
    } // namespace detail

    /// \brief Parse the value from string using template
    /// \tparam Ty type that it should return
    /// \param s string
    /// \return parsed value
    template <typename Ty, typename Ctx = detail::serializer_ctx<Ty>>
    [[nodiscard]] Ty parse(const std::string_view s) {
        if constexpr (Ctx::is_int32) {
            return parse_int32(s);
        } else if constexpr (Ctx::is_uint32) {
            return parse_uint32(s);
        } else if constexpr (Ctx::is_bool) {
            return parse_bool(s);
        } else if constexpr (Ctx::is_int8) {
            return parse_int8(s);
        } else if constexpr (Ctx::is_uint8) {
            return parse_uint8(s);
        } else {
            static_assert(types::always_false_v<Ty>, "Unknown type to parse");
            return {};
        }
    }

    /// \brief Serialize value to string
    /// \tparam Ty type that we're serializing
    /// \param value value that we should serialize
    /// \return serialized value
    template <typename Ty, typename Ctx = detail::serializer_ctx<Ty>>
    [[nodiscard]] std::string serialize(const Ty value) {
        if constexpr (Ctx::is_number) {
            return std::to_string(value);
        } else if constexpr (Ctx::is_bool) {
            return value ? "true" : "false";
        } else {
            static_assert(types::always_false_v<Ty>, "Unknown type to serialize");
            return {};
        }
    }

    /// \brief Parse string to the `out` type and store it in the std::any ref
    /// \param out output reference
    /// \param s string that it should parse
    inline void parse_to_any(std::any& out, const std::string_view s) {
        assert(out.has_value());

        const auto hash = out.type().hash_code();

        /// This is ugly, however, i can't calculate typeid(T).hash_code() in constexpr so
        /// no optimized switch cases :shrug: (could be implementation defined)
#define MAKE_CASE(type)                     \
    if (hash == typeid(type).hash_code()) { \
        out.emplace<type>(parse<type>(s));  \
        return;                             \
    }

        MAKE_CASE(int)
        MAKE_CASE(unsigned int)
        MAKE_CASE(long)
        MAKE_CASE(unsigned long)
        MAKE_CASE(bool)

#undef MAKE_CASE

        throw std::runtime_error(std::format("parse_to_any: Unable to parse '{}' -> unsupported type", s));
    }

    /// \brief Serialize any value to string
    /// \param ref any reference
    /// \return serialized string
    [[nodiscard]] inline std::string serialize_any(const std::any& ref) {
        assert(ref.has_value());
        const auto hash = ref.type().hash_code();

/// This is ugly, however, i can't calculate typeid(T).hash_code() in constexpr so
/// no optimized switch cases :shrug: (could be implementation defined)
#define MAKE_CASE(type)                                   \
    if (hash == typeid(type).hash_code()) {               \
        return serialize<type>(std::any_cast<type>(ref)); \
    }

        MAKE_CASE(int)
        MAKE_CASE(unsigned int)
        MAKE_CASE(long)
        MAKE_CASE(unsigned long)
        MAKE_CASE(bool)

#undef MAKE_CASE

        throw std::runtime_error(std::format("parse_to_any: Unable to serialize -> unsupported type"));
    }
} // namespace util::string
