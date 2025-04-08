#pragma once
#include <func_parser/map/map.hpp>
#include <func_parser/parser.hpp>
#include <tests_util.hpp>

namespace func_parser_util {
    [[nodiscard]] inline auto get_sections(std::span<std::uint8_t> pe_bytes) {
        return test::with_image(pe_bytes, []<pe::any_image_t Img>(Img image) -> std::vector<pe::section_t> { return image.sections; });
    }

    [[nodiscard]] inline const func_parser::function_t* get_function(const func_parser::function_list_t& list, const std::string_view name) {
        const auto it = std::ranges::find_if(list, [name_ = name](const auto& f) { return f.name == name_; });
        if (it == list.end()) {
            EXPECT_TRUE(false) << "Function " << name << " not found";
            return nullptr;
        }
        return &*it;
    }

    using expected_func_t = std::tuple<std::string_view, std::ptrdiff_t>;

    template <size_t N>
    void expect_funcs(const func_parser::function_list_t& list, const std::array<expected_func_t, N>& expected) {
        for (const auto& [name, rva] : expected) {
            auto* func = get_function(list, name);
            if (func == nullptr) {
                /// We will report error within the get_function
                continue;
            }
            EXPECT_EQ(func->rva, rva) << "Function " << name << " expected to be at " << std::showbase << std::hex << rva;
        }
    }

    template <size_t N>
    void run_map_test(const std::string_view folder_name, const std::string_view exe_name, const std::string_view map_name,
                      const std::size_t expected_size, const std::array<expected_func_t, N>& expected) {
        OBFUSCATOR_TEST_START;

        auto binary = test::read_resource("constant-moving", folder_name, exe_name);
        const auto map_path = test::get_resource("constant-moving", folder_name, map_name);

        const auto sections = get_sections(std::span(binary.data(), binary.size()));
        const auto functions = func_parser::map::discover_functions(map_path, sections);

        ASSERT_EQ(functions.size(), expected_size);
        expect_funcs(functions, expected);
    }
} // namespace func_parser_util