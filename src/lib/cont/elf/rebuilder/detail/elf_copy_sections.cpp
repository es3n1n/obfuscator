#include "cont/elf/rebuilder/rebuilder.hpp"

namespace cont::elf::detail {
    void copy_sections(Image* image, std::vector<std::uint8_t>& result) {
        for (auto& section : image->sections) {
            if (section.size_raw_data == 0 || section.symbolic) {
                continue;
            }

            /// Copy the section raw data to the result
            const auto src = memory::address(section.raw_data.data());
            const auto dst = memory::address(result.data()).offset(section.ptr_raw_data);

            std::memcpy(dst.ptr(), src.ptr(), section.size_raw_data);
        }
    }
} // namespace cont::elf::detail
