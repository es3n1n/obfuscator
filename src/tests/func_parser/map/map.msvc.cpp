#include "tests_util.hpp"

#include <func_parser/map/map.hpp>

TEST(MapMSVC, x64) {
    OBFUSCATOR_TEST_START;
    auto symbols = func_parser::map::discover_functions(test::get_resource("map", "msvc_x64.map"));

    ASSERT_EQ(symbols.size(), 118);

    const auto sym_atexit =
        std::find_if(symbols.begin(), symbols.end(), [](const func_parser::function_t& function) -> bool { return function.name == "atexit"; });

    const auto sym_main =
        std::find_if(symbols.begin(), symbols.end(), [](const func_parser::function_t& function) -> bool { return function.name == "main"; });

    ASSERT_NE(sym_atexit, symbols.end());
    ASSERT_NE(sym_main, symbols.end());

    ASSERT_EQ(sym_atexit->rva, 0x1668);
    ASSERT_EQ(sym_main->rva, 0x1000);
}

TEST(MapMSVC, x86) {
    /// TODO
    ASSERT_EQ(1, 2);
}
