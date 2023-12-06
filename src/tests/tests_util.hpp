#pragma once
#include <filesystem>

#include <util/files.hpp>
#include <util/types.hpp>

#include <gtest/gtest.h>

#define OBFUSCATOR_TEST_START test::start()
#define OBFUSCATOR_TEST_OPT_V(v) v.has_value() && v.value()

namespace test {
    // Common test startup stuff
    //
    inline void start() noexcept {
        logger::enabled = false;
    }

    inline std::filesystem::path get_resources_dir() noexcept {
        return OBFUSCATOR_RESOURCES_PATH;
    }

    // get_resource("some_dir", "some_file.dat") -> /resources/some_dir/some_file.dat
    //
    template <typename... TArgs>
    std::filesystem::path get_resource(TArgs... args) {
        // Converting variadic arguments to array
        //
        const auto paths = types::to_array(std::forward<TArgs>(args)...);

        // Resolving path
        //
        std::filesystem::path path = get_resources_dir();
        for (auto& child : paths) {
            path = path / child;
        }

        return path;
    }

    // read_resource("some_dir", "some_file.dat") -> file data from /resources/some_dir/some_file.dat
    //
    template <typename... TArgs>
    std::vector<std::uint8_t> read_resource(TArgs... args) {
        // Resolving path
        //
        const std::filesystem::path path = get_resource(std::forward<TArgs>(args)...);

        // Read the file and return as a result
        //
        return util::read_file(path.wstring());
    }
} // namespace test
