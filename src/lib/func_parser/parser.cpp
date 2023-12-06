#include "func_parser/parser.hpp"
#include "func_parser/common/combiner.hpp"
#include "func_parser/map/map.hpp"
#include "func_parser/pdb/pdb.hpp"
#include "util/logger.hpp"

namespace func_parser {
    template <pe::any_image_t Img>
    Instance<Img>::~Instance() {
        function_list_.clear();
        function_lists_.clear();
    }

    template <pe::any_image_t Img>
    void Instance<Img>::collect_functions() {
        // Parsing from all sources possible
        //
        parse();

        // Combining and sanitizing results
        //
        function_list_ = combiner::combine_function_lists(function_lists_);
        function_list_ = sanitizer::sanitize_function_list(function_list_, image_);

        // If 0 functions found
        //
        if (function_list_.empty()) {
            throw std::runtime_error("parser: Parsed 0 functions in total");
        }

        logger::debug("func_parser: discovered {} functions", function_list_.size());
    }

    template <pe::any_image_t Img>
    void Instance<Img>::parse() {
        parse_pdb();
        parse_map();
    }

    template <pe::any_image_t Img>
    void Instance<Img>::parse_pdb() {
        // If force disabled
        //
        if (!config_.pdb_enabled) {
            return;
        }

        // Obtaining base of code
        //
        const auto base_of_code = image_->raw_image->get_nt_headers()->optional_header.base_of_code;

        // Trying to parse from a custom pdb path first
        //
        if (config_.pdb_path.has_value()) {
            if (push(pdb::discover_functions(config_.pdb_path.value(), base_of_code))) {
                return;
            }
        }

        // Trying to parse from a codeview path
        //
        if (push(pdb::discover_functions(image_->find_codeview70(), base_of_code))) {
            return;
        }

        // Trying to find .pdb near the executable
        //
        auto pdb_path = obfuscator_config_.binary_path;
        pdb_path = pdb_path.replace_extension(".pdb");
        push(pdb::discover_functions(pdb_path, base_of_code));
    }

    template <pe::any_image_t Img>
    void Instance<Img>::parse_map() {
        // If force disabled
        //
        if (!config_.map_enabled) {
            return;
        }

        // Trying to parse from a custom path first
        //
        if (config_.map_path.has_value()) {
            if (push(map::discover_functions(config_.map_path.value(), image_->sections))) {
                return;
            }
        }

        // Trying to find .map file near the binary
        //
        auto map_path = obfuscator_config_.binary_path;
        map_path = map_path.replace_extension(".map");
        push(map::discover_functions(map_path, image_->sections));
    }

    PE_DECL_TEMPLATE_CLASSES(Instance);
} // namespace func_parser