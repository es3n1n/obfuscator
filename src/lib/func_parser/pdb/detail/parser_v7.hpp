#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "func_parser/pdb/detail/structs.hpp"
#include <es3n1n/common/memory/address.hpp>
#include <es3n1n/common/types.hpp>
#include <util/structs.hpp>

// \todo: @es3n1n: we should probably use OMAP from the pdb for rva conversion

namespace func_parser::pdb::detail {
    class V7Parser {
    public:
        V7Parser(const std::uint8_t* pdb_data, const size_t pdb_size): pdb_data_(pdb_data), pdb_size_(pdb_size) {
            read_header();
        }

        DEFAULT_DTOR(V7Parser);
        DEFAULT_COPY(V7Parser);

        void iter_symbols(const std::function<bool(std::uint16_t, memory::address)>& callback) const;

        template <typename Ty = DBIRecordHeader>
        void iter_symbols(const std::uint16_t kind, const std::function<void(Ty*)>& callback) const {
            iter_symbols([=](const std::uint16_t it_kind, const memory::address raw) -> bool {
                if (it_kind != kind) {
                    return true;
                }

                callback(raw.as<Ty*>());
                return true;
            });
        }

        template <typename Ty = DBIRecordHeader, typename... Args>
        void iter_symbols(const std::function<void(Ty*)>& callback, Args... args) const {
            for (auto kind : types::to_array(std::forward<Args>(args)...)) {
                iter_symbols(kind, callback);
            }
        }

    protected:
        void read_header();
        void read_streams();
        void read_dbi();

    public:
        [[nodiscard]] std::optional<std::uint64_t> get_section(const std::size_t num) const noexcept {
            if (sections_.size() <= num) {
                return std::nullopt;
            }

            return sections_.at(num);
        }

    private:
        memory::address pdb_data_ = memory::address(nullptr);
        [[maybe_unused]] size_t pdb_size_ = 0;

        SuperBlock* header_ = nullptr;

        // contains virtual addresses, cba storing anything else
        std::vector<std::uint64_t> sections_;

        // these streams should be in the right order
        std::vector<std::vector<std::uint8_t>> streams_;

        // key is sym kind, values are ptrs to the symbols
        std::unordered_map<std::uint16_t, std::vector<memory::address>> dbi_symbols_;
    };
} // namespace func_parser::pdb::detail
