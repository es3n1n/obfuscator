#pragma once
#include "analysis/common/common.hpp"
#include <es3n1n/common/files.hpp>

#include <sstream>

#include <magic_enum.hpp>

namespace analysis::debug {
    namespace detail {
        constexpr std::string_view unknown_type_name = "Unknown";

        const std::unordered_map<std::size_t, std::string_view> operand_type_lookup = {
            {0, std::string_view{"None"}}, {1, std::string_view{"Reg"}},   {2, std::string_view{"Mem"}},
            {3, std::string_view{"Imm"}},  {4, std::string_view{"Label"}},
        };
    } // namespace detail

    inline void serialize_bb_to_file(const bb_t& basic_block, const std::filesystem::path& file) {
        std::stringstream ss;

        ss << "start:" << std::hex << basic_block.start_rva.value_or(0).inner() << '\n';
        ss << "end:" << std::hex << basic_block.start_rva.value_or(0).inner() << '\n';

        for (const auto& instruction : basic_block.instructions) {
            ss << "instruction:" << std::hex << instruction->rva.value_or(0).inner() << ";"
               << ZydisMnemonicGetString(static_cast<ZydisMnemonic>(instruction->ref->getMnemonic().value())) << '\n';
        }

        for (const auto& successor : basic_block.successors) {
            ss << "successor:" << std::hex << successor->start_rva.value_or(0).inner() << '\n';
        }

        for (const auto& predecessor : basic_block.predecessors) {
            ss << "predecessor:" << std::hex << predecessor->start_rva.value_or(0).inner() << '\n';
        }

        const auto data = ss.str();
        files::write_file(file, reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

    [[maybe_unused]] inline void dump_bb(const bb_t& bb) {
        logger::info("BB: {:#x} :: {:#x}", bb.start_rva.value_or(0), bb.end_rva.value_or(0));
        logger::info<1>("Valid: {}", bb.flags.valid);
        logger::info<1>("Instructions:");

        for (const auto& instruction : bb.instructions) {
            logger::info<2>("{:#x}: {}", instruction->rva.value_or(0), //
                            ZydisMnemonicGetString(static_cast<ZydisMnemonic>(instruction->ref->getMnemonic().value())));

            if (const auto ops_count = instruction->ref->getOperandCount(); ops_count > 0) {
                logger::info<3>("Operands:");

                for (std::size_t i = 0; i < ops_count; ++i) {
                    const auto operand = instruction->ref->getOperand(i);

                    const auto operand_name_pair = detail::operand_type_lookup.find(operand.getTypeIndex());
                    const auto operand_name = //
                        operand_name_pair == std::end(detail::operand_type_lookup) ? detail::unknown_type_name : operand_name_pair->second;

                    logger::info<4>("{}", operand_name);
                }
            }

            if (!instruction->cf.empty()) {
                logger::info<3>("CF:");
                for (const auto& cf : instruction->cf) {
                    logger::info<4>("{}: {:#x}", magic_enum::enum_name<cf_direction_t::e_type>(cf.type), *cf.bb->start_rva);
                }
            }

            if (instruction->reloc.type != insn_reloc_t::e_type::NONE) {
                logger::info<3>("Relocation:");
                logger::info<4>("TYPE_{}: {:#x} | Offset: {}", magic_enum::enum_name<insn_reloc_t::e_type>(instruction->reloc.type),
                                instruction->reloc.imm_rva,
                                instruction->reloc.offset.has_value() ? std::to_string(instruction->reloc.offset.value()) : "none");
            }
        }

        if (!bb.successors.empty()) {
            logger::info<1>("Successors:");

            for (const auto& successor : bb.successors) {
                logger::info<2>("{:#x}", successor->start_rva.value_or(0));
            }
        }

        if (!bb.predecessors.empty()) {
            logger::info<1>("Predecessors:");

            for (const auto& predecessor : bb.predecessors) {
                logger::info<2>("{:#x}", predecessor->start_rva.value_or(0));
            }
        }
    }
} // namespace analysis::debug
