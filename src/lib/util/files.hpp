#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace util {
    inline std::vector<std::uint8_t> read_file(const std::filesystem::path& path) {
        std::fstream file(path, std::ios::in | std::ios::binary);
        if (!file) {
            return {};
        }

        file.seekg(0, std::fstream::end);
        const auto f_size = file.tellg();
        file.seekg(0, std::fstream::beg);

        std::vector<uint8_t> buffer(static_cast<const unsigned int>(f_size));

        // NOLINTNEXTLINE
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

        return buffer;
    }

    inline void write_file(const std::filesystem::path& path, const std::uint8_t* raw_buffer, const size_t buffer_size) {
        std::ofstream file(path, std::ios::binary | std::ios::out);
        // NOLINTNEXTLINE
        file.write(reinterpret_cast<const char*>(raw_buffer), buffer_size);
        file.close();
    }
} // namespace util