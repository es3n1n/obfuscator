#pragma once
#include <es3n1n/common/memory/address.hpp>

namespace types {
    using rva_t = memory::address;

    struct range_t {
        rva_t start;
        rva_t end;

        [[nodiscard]] std::size_t size() const {
            return (end - start).as<std::size_t>();
        }
    };
} // namespace types
