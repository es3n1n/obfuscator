#include "func_parser/func_parser_util.hpp"
#include "tests_util.hpp"

namespace {
    constexpr std::string_view kExeName = "TestProject.exe";
    constexpr std::string_view kMapName = "ida.map";
} // namespace

TEST(FuncParserMap__IDA, ClangX64) {
    func_parser_util::run_map_test("clang_x64", kExeName, kMapName, 313,
                                   std::to_array<func_parser_util::expected_func_t>({
                                       {"main", 0x1000},
                                       {"__scrt_common_main_seh", 0x13d0},
                                   }));
}

TEST(FuncParserMap__IDA, ClangX86) {
    func_parser_util::run_map_test("clang_x86", kExeName, kMapName, 274,
                                   std::to_array<func_parser_util::expected_func_t>({
                                       {"_main", 0x1000},
                                       {"__scrt_common_main_seh", 0x130c},
                                   }));
}

TEST(FuncParserMap__IDA, MSVCX64) {
    func_parser_util::run_map_test("msvc_x64", kExeName, kMapName, 319,
                                   std::to_array<func_parser_util::expected_func_t>({
                                       {"main", 0x1070},
                                       {"__scrt_common_main_seh", 0x1224},
                                   }));
}

TEST(FuncParserMap__IDA, MSVCX86) {
    func_parser_util::run_map_test("msvc_x86", kExeName, kMapName, 283,
                                   std::to_array<func_parser_util::expected_func_t>({
                                       {"_main", 0x1040},
                                       {"__scrt_common_main_seh", 0x11a1},
                                   }));
}
