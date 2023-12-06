#pragma once
#include "util/structs.hpp"

#include <cstdint>
#include <expected>
#include <zasm/zasm.hpp>

namespace easm {
    constexpr zasm::MachineMode kDefaultMm = zasm::MachineMode::AMD64;
    constexpr size_t kDefaultSize = 15;

    class Decoder {
    public:
        DEFAULT_DTOR(Decoder);
        DEFAULT_COPY(Decoder);
        explicit Decoder(const zasm::MachineMode machine_mode = kDefaultMm): machine_mode_(machine_mode), decoder_(machine_mode_) { }

        zasm::Program decode_block(const std::uint8_t* data, std::size_t size, uint64_t orig_address = 0ULL);
        std::expected<zasm::InstructionDetail, zasm::Error> decode_insn_detail(const std::uint8_t* data, size_t size = kDefaultSize,
                                                                               uint64_t orig_address = 0ULL);
        std::expected<zasm::Instruction, zasm::Error> decode_insn(const std::uint8_t* data, size_t size = kDefaultSize, uint64_t orig_address = 0ULL);

    private:
        zasm::MachineMode machine_mode_;
        zasm::Decoder decoder_;
    };

    zasm::Program decode_block(const std::uint8_t* data, std::size_t size = kDefaultSize, zasm::MachineMode machine_mode = kDefaultMm,
                               uint64_t orig_address = 0ULL);

    std::expected<zasm::InstructionDetail, zasm::Error> decode_insn_detail(const std::uint8_t* data, size_t size = kDefaultSize,
                                                                           zasm::MachineMode machine_mode = kDefaultMm, uint64_t orig_address = 0ULL);

    std::expected<zasm::Instruction, zasm::Error> decode_insn(const std::uint8_t* data, size_t size = kDefaultSize,
                                                              zasm::MachineMode machine_mode = kDefaultMm, uint64_t orig_address = 0ULL);
} // namespace easm
