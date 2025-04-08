#include "func_parser/pdb/pdb.hpp"
#include "func_parser/pdb/detail/parser_v7.hpp"
#include <es3n1n/common/files.hpp>
#include <es3n1n/common/logger.hpp>

namespace func_parser::pdb {
    function_list_t discover_functions(const std::filesystem::path& pdb_path, const std::uint64_t base_of_code [[maybe_unused]]) {
        // Return an empty set if file doesn't exist
        //
        if (!exists(pdb_path)) {
            return {};
        }

        // Reading file
        //
        const auto pdb_content = files::read_file(pdb_path);
        if (!pdb_content.has_value() || pdb_content->empty()) [[unlikely]] {
            return {};
        }

        // If magic doesn't equal to pdb7 magic then sorry we cannot parse this pdb
        // \todo: @es3n1n: add pdb2 support
        //
        if (std::memcmp(pdb_content->data(), detail::kMicrosoftPdb7Magic.data(), detail::kMicrosoftPdb7Magic.size()) != 0) {
            FIXME_NO_ARG(1, "Only PDB7 is supported atm");
            return {};
        }

        // Initialize parser
        //
        function_list_t result = {};
        const detail::V7Parser parser(pdb_content->data(), pdb_content->size());

        /// Iterate procedures
        parser.iter_symbols<detail::DBIFunction>(
            [&result, parser](detail::DBIFunction* sym) -> void { // NOLINT(bugprone-exception-escape)
                auto& item = result.emplace_back();

                item.valid = true;
                item.name = static_cast<std::string>(sym->Name); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
                item.size = std::make_optional<std::size_t>(sym->Size);

                const auto segment = parser.get_section(sym->Segment - 1);
                if (!segment.has_value()) {
                    item.valid = false;
                    FIXME(1, "pdb: unable to obtain segment {}", sym->Segment);
                    return;
                }

                item.rva = *segment + sym->Offset;
            },
            detail::e_symbol_kind::S_LPROC32, // Iterating over local procedures
            detail::e_symbol_kind::S_GPROC32 // Iterating over global procedures
        );

        /// Iterate public symbols
        parser.iter_symbols<detail::DBIPublicSymbol>(
            [&result, parser](detail::DBIPublicSymbol* sym) -> void {
                /// We are looking only for functions
                if ((sym->Flags & detail::PublicSymFlags::Function) == 0U) {
                    return;
                }

                /// Inserting new function
                auto& item = result.emplace_back();
                item.valid = true;
                item.name = static_cast<std::string>(sym->Name); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)

                /// Looking for segment
                const auto segment = parser.get_section(sym->Segment - 1);
                if (!segment.has_value()) {
                    FIXME(1, "pdb: unable to obtain segment {}", sym->Segment);
                    return;
                }

                item.rva = *segment + sym->Offset;
            },
            detail::e_symbol_kind::S_PUB32 // Public symbols
        );

        /// \todo: @es3n1n: S_EXPORT

        // We are done here
        //
        return result;
    }
} // namespace func_parser::pdb
