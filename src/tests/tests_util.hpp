#pragma once
#include <filesystem>

#include <es3n1n/common/files.hpp>
#include <es3n1n/common/random.hpp>
#include <obfuscator/transforms/scheduler.hpp>

#include <pe/arch/arch.hpp>
#include <pe/pe.hpp>

#include <gtest/gtest.h>

#define OBFUSCATOR_TEST_START test::start()
#define OBFUSCATOR_TEST_OPT_V(v) v.has_value() && v.value()

namespace test {
    // Common test startup stuff
    //
    inline void start() noexcept {
        logger::enabled = false;
        rnd::detail::seed();
        obfuscator::startup_scheduler();
    }

    [[nodiscard]] inline std::filesystem::path get_resources_dir() noexcept {
        return OBFUSCATOR_RESOURCES_PATH;
    }

    // get_resource("some_dir", "some_file.dat") -> /resources/some_dir/some_file.dat
    //
    template <typename... TArgs>
    [[nodiscard]] std::filesystem::path get_resource(TArgs... args) {
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
    [[nodiscard]] std::vector<std::uint8_t> read_resource(TArgs... args) {
        // Resolving path
        //
        const std::filesystem::path path = get_resource(std::forward<TArgs>(args)...);

        // Read the file and return as a result
        //
        return files::read_file(path.wstring()).value();
    }

    /// \todo @es3n1n: std::invoke_result
    template <typename Callable>
    [[nodiscard]] auto with_image(const std::span<std::uint8_t> binary, Callable&& callable) {
        auto* const x64 = reinterpret_cast<win::image_x64_t*>(binary.data());
        auto* const x86 = reinterpret_cast<win::image_x86_t*>(binary.data());

        if (pe::arch::is_x64(x64)) {
            return callable(pe::Image(x64));
        }

        return callable(pe::Image(x86));
    }
} // namespace test
