#include "func_parser/pdb/pdb.hpp"
#include "func_parser_util.hpp"

namespace {
    constexpr std::string_view kExeName = "TestProject.exe";
    constexpr std::string_view kPdbName = "TestProject.pdb";

    template <size_t N>
    void run_pdb_test(const std::string_view folder_name, const std::string_view exe_name, const std::string_view pdb_name,
                      const std::size_t expected_size, const std::array<func_parser_util::expected_func_t, N>& expected) {
        OBFUSCATOR_TEST_START;

        auto binary = test::read_resource("constant-moving", folder_name, exe_name);

        const auto base_of_code = test::with_image(std::span(binary.data(), binary.size()), []<pe::any_image_t Img>(Img image) -> std::uintptr_t {
            return image.raw_image->get_nt_headers()->optional_header.base_of_code;
        });

        const auto pdb_path = test::get_resource("constant-moving", folder_name, pdb_name);
        const auto functions = func_parser::pdb::discover_functions(pdb_path, base_of_code);

        ASSERT_EQ(functions.size(), expected_size);
        func_parser_util::expect_funcs(functions, expected);
    }
} // namespace

TEST(FuncParserPDB__Compilers, ClangX64) {
    run_pdb_test("clang_x64", kExeName, kPdbName, 136,
                 std::to_array<func_parser_util::expected_func_t>({
                     {"main", 0x1000},
                     {"__scrt_initialize_crt", 0x16ac},
                 }));
}

TEST(FuncParserPDB__Compilers, ClangX86) {
    run_pdb_test("clang_x86", kExeName, kPdbName, 140,
                 std::to_array<func_parser_util::expected_func_t>({
                     {"_main", 0x1000},
                     {"___scrt_initialize_crt", 0x15be},
                 }));
}

TEST(FuncParserPDB__Compilers, MSVCX64) {
    run_pdb_test("msvc_x64", kExeName, kPdbName, 136,
                 std::to_array<func_parser_util::expected_func_t>({
                     {"main", 0x1070},
                     {"__vcrt_initialize", 0x1840},
                 }));
}

TEST(FuncParserPDB__Compilers, MSVCX86) {
    run_pdb_test("msvc_x86", kExeName, kPdbName, 140,
                 std::to_array<func_parser_util::expected_func_t>({
                     {"_main", 0x1040},
                     {"___vcrt_initialize", 0x1751},
                 }));
}
