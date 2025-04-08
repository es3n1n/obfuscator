#pragma once
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <vector>

namespace func_parser {
    struct function_t {
        constexpr function_t() = default;

        bool valid = false;
        std::string name;
        std::uint64_t rva = 0;
        std::optional<std::size_t> size = std::nullopt;

        // \note: @es3n1n: im not really sure how to properly merge stuff like
        // names. technically there could be a different size of function in the
        // different sources, but I'll let the future me to decide on this
        void merge(const function_t& another) {
            // Trying to merge size
            //
            if (!this->size.has_value() && another.size.has_value()) {
                this->size = std::make_optional<std::size_t>(another.size.value());
            }

            // Copying name if item is in a valid state now
            //
            if (!this->valid && another.valid) {
                this->valid = another.valid;
                this->name = another.name;
            }
        }
    };

    using function_list_t = std::vector<function_t>;
} // namespace func_parser

template <>
struct std::formatter<func_parser::function_t> : std::formatter<std::string> {
    template <class FormatContextTy>
    constexpr auto format(const func_parser::function_t& inst, FormatContextTy& ctx) const {
        return std::formatter<std::string>::format( //
            std::format("valid[{}] name[{}] size[{}] rva[{:#x}]", inst.valid, inst.name,
                        inst.size.has_value() ? std::to_string(inst.size.value()) : "none", inst.rva),
            ctx);
    }
};
