#pragma once
#include "memory/address.hpp"
#include "util/structs.hpp"
#include <array>

namespace types {
    template <class Ty, class... Types>
    inline constexpr bool is_any_of_v = std::disjunction_v<std::is_same<Ty, Types>...>;

    template <class... Args>
    auto to_array(Args&&... args) {
        return std::array<std::common_type_t<Args...>, sizeof...(Args)>{std::forward<Args>(args)...};
    }

    using rva_t = memory::address;

    struct range_t {
        rva_t start;
        rva_t end;
    };

    template <typename T>
    class Singleton {
    protected:
        DEFAULT_CTOR_DTOR(Singleton);
        NON_COPYABLE(Singleton);

    public:
        [[nodiscard]] static T& get() {
            static T instance = {};
            return instance;
        }
    };

    /// Used for static assertions
    template <typename = std::monostate> concept always_false_v = false;
} // namespace types
