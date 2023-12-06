#pragma once
#include "util/memory/address.hpp"
#include <expected>
#include <list>
#include <vector>
#include <zasm/zasm.hpp>

namespace easm {
    struct assembled_t {
        std::vector<std::uint8_t> data;
        std::list<zasm::RelocationInfo> relocations;
    };

    std::size_t estimate_program_size(const zasm::Program& program);
    assembled_t assemble_program(memory::address base_address, const zasm::Program& program);
    std::expected<std::vector<std::uint8_t>, zasm::Error> encode_jmp(zasm::MachineMode machine_mode, memory::address source, memory::address destination);
} // namespace easm
