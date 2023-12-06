#include "easm/assembler/assembler.hpp"

namespace easm {
    constexpr auto kByteSizeInBits = 8;

    std::size_t estimate_program_size(const zasm::Program& program) {
        std::size_t result = 0;

        //
        // Iterating program nodes
        //
        for (auto* node = program.getHead(); node != nullptr; node = node->getNext()) {
            //
            // Handling `zasm::Data`
            //
            if (const auto* node_data = node->getIf<zasm::Data>(); node_data != nullptr) {
                result += node_data->getTotalSize();
                continue;
            }

            //
            // Handling `zasm::Instruction`
            //
            if (const auto* node_insn = node->getIf<zasm::Instruction>(); node_insn != nullptr) {
                const auto& insn_info = node_insn->getDetail(program.getMode());

                if (!insn_info) {
                    throw std::runtime_error("Unable to estimate program size: unable to get if instr info");
                }

                result += insn_info->getLength();
                continue;
            }

            //
            // Handling `zasm::EmbeddedLabel`
            //
            if (const auto* embedded_label = node->getIf<zasm::EmbeddedLabel>(); embedded_label != nullptr) {
                result += getBitSize(embedded_label->getSize()) / kByteSizeInBits;
            }
        }

        return result;
    }

    assembled_t assemble_program(const memory::address base_address, const zasm::Program& program) {
        zasm::Serializer serializer = {};

        // Serializing program
        //
        if (const auto err = serializer.serialize(program, base_address.as<std::int64_t>()); err != zasm::Error::None) {
            throw std::runtime_error(std::format("Unable to serialize program: {}", getErrorName(err)));
        }

        // Copying the result buffer
        //
        assembled_t result = {};
        result.data.resize(serializer.getCodeSize());
        std::memcpy(result.data.data(), serializer.getCode(), result.data.size());

        // Store relocations
        //
        for (std::size_t i = 0; i < serializer.getRelocationCount(); ++i) {
            result.relocations.emplace_back(*serializer.getRelocation(i));
        }

        return result;
    }

    std::expected<std::vector<std::uint8_t>, zasm::Error> encode_jmp(const zasm::MachineMode machine_mode, const memory::address source,
                                                                     const memory::address destination) {
        zasm::Program program(machine_mode);
        zasm::x86::Assembler assembler(program);

        assembler.jmp(zasm::Imm(destination.as<std::uint64_t>()));

        zasm::Serializer serializer;
        if (auto res = serializer.serialize(program, source.as<std::int64_t>()); res != zasm::Error::None) {
            return std::unexpected(res);
        }

        std::vector<std::uint8_t> result = {};
        result.resize(serializer.getCodeSize());
        std::memcpy(result.data(), serializer.getCode(), result.size());

        return result;
    }
} // namespace easm
