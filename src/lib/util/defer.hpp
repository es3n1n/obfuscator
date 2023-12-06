#pragma once
#include <memory>

#define CAT_(x, y) x##y
#define CAT(x, y) CAT_(x, y)

#define defer auto CAT(_defer_instance_, __COUNTER__) = defer_::defer_{} % [&]()

namespace defer_ {
    template <typename callable>
    struct type {
        callable cb;

        explicit type(callable&& _cb): cb(std::forward<callable>(_cb)) { }

        ~type() {
            cb();
        }
    };

    struct defer_ {
        template <typename callable>
        type<callable> operator%(callable&& cb) {
            return type<callable>{std::forward<callable>(cb)};
        }
    };
} // namespace defer_
