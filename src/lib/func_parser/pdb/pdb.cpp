#include "func_parser/pdb/pdb.hpp"
#include "func_parser/pdb/detail/parser_v7.hpp"
#include "util/logger.hpp"

namespace func_parser::pdb {
    function_list_t discover_functions(const std::filesystem::path& pdb_path, const std::uint64_t) {
        // Return an empty set if file doesn't exist
        //
        if (!exists(pdb_path)) {
            return {};
        }

        // Reading file
        //
        const auto pdb_content = util::read_file(pdb_path);
        if (pdb_content.empty()) [[unlikely]] {
            return {};
        }

        // If magic doesn't equal to pdb7 magic then sorry we cannot parse this pdb
        // @todo: @es3n1n: add pdb2 support
        //
        if (std::memcmp(pdb_content.data(), detail::kMicrosoftPdb7Magic.data(), detail::kMicrosoftPdb7Magic.size()) != 0) {
            FIXME_NO_ARG(1, "Only PDB7 is supported atm");
            return {};
        }

        // We gamin
        //
        function_list_t result = {};

        const detail::V7Parser parser(pdb_content.data(), pdb_content.size());
        parser.iter_symbols<detail::DBIRecordProc32>(
            [&result, parser](detail::DBIRecordProc32* sym) -> void {
                auto& item = result.emplace_back();
                item.valid = true; // probably we should check for something first

                item.name = sym->Name;
                item.size = std::make_optional<std::size_t>(sym->Size);

                const auto segment = parser.get_section(sym->Segment - 1);
                if (!segment.has_value()) {
                    item.valid = false;
                    logger::warn("pdb: Unable to obtain segment base num[{}] func[{}]", sym->Segment, sym->Name);
                    return;
                }

                item.rva = segment.value() + sym->Offset;
            },
            detail::e_symbol_kind::S_LPROC32, // Iterating over local procedures
            detail::e_symbol_kind::S_GPROC32 // Iterating over global procedures
        );

        // We are done here
        //
        return result;
    }
} // namespace func_parser::pdb
