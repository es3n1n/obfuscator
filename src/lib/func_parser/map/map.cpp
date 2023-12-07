#include <regex>

#include "func_parser/map/map.hpp"
#include "util/logger.hpp"
#include "util/memory/casts.hpp"

#include <cassert>

namespace func_parser::map {
    namespace {

        // Extracting `0001` segment index, `00000000` segment offset and `main` from
        // `0001:00000000 main 0000000140001000 f FileName.obj`
        //
        const std::regex SYMBOL_REGEX(R"(([0-9a-fA-F]+):([0-9a-fA-F]+)\s+([^\s]+))");

        std::uint64_t parse_hex_string(const std::string& value) {
            // NOLINTNEXTLINE
            return std::stoull(value, nullptr, 16);
        }

        std::uint32_t parse_decimal_string(const std::string& value) {
            // NOLINTNEXTLINE
            return std::stoul(value, nullptr, 10);
        }
    } // namespace

    function_list_t discover_functions(const std::filesystem::path& map_path, const std::vector<pe::section_t>& sections) {
        // Reading map file
        //
        const auto map_content = util::read_file(map_path);
        if (map_content.empty()) {
            throw std::runtime_error("Empty map file");
        }

        // Converting to string stream
        //
        std::stringstream str_stream;
        str_stream.str(std::string{memory::cast<const char*>(map_content.data()), map_content.size()});

        // Initializing func_parser state
        //
        function_list_t result;
        bool parsing_symbols = false;

        // Initializing regex matches
        //
        std::smatch matches;

        for (std::string line; std::getline(str_stream, line);) {
            // Skipping empty lines
            //
            if (line.empty()) {
                continue;
            }

            // If we are not parsing symbols, then we should try to find the
            // start marker of symbols first
            // `Address Publics by Value Rva+Base Lib:Object`
            //
            if (!parsing_symbols) {
                parsing_symbols = line.find("ddress") != std::string::npos && //
                                  line.find("ublics by Value") != std::string::npos;
                continue;
            }

            // Stop once we hit the `entry point` and/or `Static symbols`
            //
            if (line.find("ntry point at") != std::string::npos || //
                line.find("tatic symbols") != std::string::npos) {
                break;
            }

            // Ok, at this point we are just parsing symbols, so we should extract
            // the VA, name and we are done
            //
            if (!std::regex_search(line, matches, SYMBOL_REGEX)) {
                continue;
            }

            // Get the section
            //
            auto section_index = parse_hex_string(matches[1]);
            if (section_index == 0) {
                continue; // garbage
            }
            section_index -= 1;
            assert(section_index < sections.size());
            const auto section = sections.at(section_index);
            const auto section_offset = parse_hex_string(matches[2]);
            const auto name = matches[3];

            // Assembling the function info
            //
            auto& function = result.emplace_back();
            function.valid = true; // :skull:
            function.size = std::nullopt;
            function.rva = section.virtual_address + section_offset;
            function.name = name;
        }

        return result;
    }
} // namespace func_parser::map
