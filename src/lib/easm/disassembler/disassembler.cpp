#include "easm/disassembler/disassembler.hpp"
#include <stdexcept>

namespace easm {
    zasm::Program Decoder::decode_block(const std::uint8_t* data, const std::size_t size, const uint64_t orig_address) {
        zasm::Program result(machine_mode_);
        zasm::x86::Assembler assembler(result);

        std::size_t decoded = 0;
        while (decoded < size) {
            const auto decoded_result = decode_insn_detail(data + decoded, size - decoded, orig_address + decoded);
            if (!decoded_result) {
                throw std::runtime_error("Unable to decode data");
            }

            if (const auto res = assembler.emit(decoded_result->getInstruction()); res != zasm::Error::None) {
                throw std::runtime_error("Unable to encode decoded data");
            }

            decoded += decoded_result->getLength();
        }

        return result;
    }

    std::expected<zasm::InstructionDetail, zasm::Error> Decoder::decode_insn_detail(const std::uint8_t* data, const size_t size,
                                                                                    const uint64_t orig_address) {
        const auto result = decoder_.decode(data, size, orig_address);

        if (!result) {
            return std::unexpected(result.error());
        }

        return result.value();
    }

    std::expected<zasm::Instruction, zasm::Error> Decoder::decode_insn(const std::uint8_t* data, const size_t size, const uint64_t orig_address) {
        const auto result = decode_insn_detail(data, size, orig_address);

        if (!result) {
            return std::unexpected(result.error());
        }

        return result->getInstruction();
    }

    //

    zasm::Program decode_block(const std::uint8_t* data, const std::size_t size, const zasm::MachineMode machine_mode, const uint64_t orig_address) {
        Decoder decoder(machine_mode);

        return decoder.decode_block(data, size, orig_address);
    }

    std::expected<zasm::InstructionDetail, zasm::Error> decode_insn_detail(const std::uint8_t* data, const zasm::MachineMode machine_mode,
                                                                           const uint64_t orig_address) {
        Decoder decoder(machine_mode);

        return decoder.decode_insn_detail(data, orig_address);
    }

    std::expected<zasm::Instruction, zasm::Error> decode_insn(const std::uint8_t* data, const zasm::MachineMode machine_mode,
                                                              const uint64_t orig_address) {
        Decoder decoder(machine_mode);

        return decoder.decode_insn(data, orig_address);
    }
} // namespace easm
