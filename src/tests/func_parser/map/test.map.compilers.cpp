#include "func_parser/func_parser_util.hpp"
#include "tests_util.hpp"

namespace {
    constexpr std::string_view kExeName = "TestProject.exe";
    constexpr std::string_view kMapName = "TestProject.map";
} // namespace

TEST(FuncParserMap__Compilers, ClangX64) {
    func_parser_util::run_map_test("clang_x64", kExeName, kMapName, 171,
                                   std::to_array<func_parser_util::expected_func_t>({
                                       {"main", 0x1000},
                                       {"__scrt_initialize_crt", 0x16ac},
                                   }));
}

TEST(FuncParserMap__Compilers, ClangX86) {
    func_parser_util::run_map_test("clang_x86", kExeName, kMapName, 163,
                                   std::to_array<func_parser_util::expected_func_t>({
                                       {"_main", 0x1000},
                                       {"___scrt_initialize_crt", 0x15be},
                                   }));
}

TEST(FuncParserMap__Compilers, MSVCX64) {
    func_parser_util::run_map_test("msvc_x64", kExeName, kMapName, 189,
                                   std::to_array<func_parser_util::expected_func_t>({
                                       {"main", 0x1070},
                                       {"__vcrt_initialize", 0x1840},
                                   }));
}

TEST(FuncParserMap__Compilers, MSVCX86) {
    func_parser_util::run_map_test("msvc_x86", kExeName, kMapName, 182,
                                   std::to_array<func_parser_util::expected_func_t>({
                                       {"_main", 0x1040},
                                       {"___vcrt_initialize", 0x1751},
                                   }));
}
