#include "cont/elf/rebuilder/rebuilder.hpp"
#include "util/sections.hpp"

#include <ranges>

namespace cont::elf::detail {
    void update_relocations(Image* image) {
        const auto img_mode = image->mode();

        /// \todo @es3n1n: Relocations need to be sorted
        ///     R_X86_64_RELATIVE first, then everything else.
        ///     count of first R_X86_64_RELATIVE should also be updated in relascount.

        /// Wiping old relocations first
        auto* rela = image->find_dynamic(DT_RELA);
        auto* rela_sz = image->find_dynamic(DT_RELASZ);
        if (rela != nullptr) {
            if (rela_sz == nullptr || rela->value == 0) {
                throw std::runtime_error("cont::elf: DT_RELA or DT_RELASZ not found or invalid");
            }

            auto* const ptr = image->rva_to_ptr(rela->value);
            std::memset(ptr, 0, rela_sz->value);
        }

        if (image->relocations.empty()) {
            image->delete_dynamic(DT_RELA);
            image->delete_dynamic(DT_RELASZ);
            return;
        }

        assert(rela != nullptr);

        /// We are not adding jmprel relocations, so dont erase them

        /// Allocate new segment for our relocations
        auto sec_hdr = sections::get(sections::e_section_t::RELOC);
        /// +1 for last DT_NULL tag
        sec_hdr.size_raw_data = (image->relocations.size() + 1) * (img_mode == ImageMode::X64 ? sizeof(Elf64_Rela) : sizeof(Elf32_Rela));
        auto& sec = image->new_section(sec_hdr);

        /// Assemble the relocation structures
        const auto assemble_structs = [&sec, &image, &img_mode]<typename Rela>() -> void {
            auto* out_ptr = memory::address(sec.raw_data.data()).ptr<Rela>();

            /// Separate them by type
            std::unordered_multimap<std::size_t, Relocation*> relocations;
            for (auto& reloc : image->relocations | std::views::values) {
                auto& info = reloc.info_raw;

                if (!info.has_value()) {
                    if (reloc.type == RelocationType::Absolute) {
                        continue;
                    }

                    switch (reloc.type) {
                    case RelocationType::HighLow: {
                        // NOLINTNEXTLINE(bugprone-branch-clone)
                        info.emplace(static_cast<std::ptrdiff_t>((img_mode == ImageMode::X64) ? R_X86_64_RELATIVE : R_386_RELATIVE));
                        break;
                    default:
                        throw std::out_of_range("cont::elf::detail::update_relocations: Unsupported relocation type for ELF");
                    }
                    }
                }

                assert(info.has_value());
                relocations.emplace(img_mode == ImageMode::X64 ? ELF64_R_TYPE(*info) : ELF32_R_TYPE(*info), &reloc);
            }

            /// Assemble R_X86_64_RELATIVE first
            for (auto& [rva, reloc] : relocations | std::views::filter([img_mode](const auto& pair) -> bool {
                                          return pair.first ==
                                                 (img_mode == ImageMode::X64 ? R_X86_64_RELATIVE : R_386_RELATIVE); // NOLINT(bugprone-branch-clone)
                                      })) {
                assert(reloc->info_raw.has_value());
                *out_ptr = Rela{
                    .r_offset = reloc->rva.template as<decltype(Rela::r_offset)>(),
                    .r_info = static_cast<decltype(Rela::r_info)>(*reloc->info_raw),
                    .r_addend = static_cast<decltype(Rela::r_addend)>(reloc->addend.value_or(0)),
                };
                ++out_ptr;
            }

            /// Assemble the rest of relocations
            for (auto& [rva, reloc] : relocations | std::views::filter([img_mode](const auto& pair) -> bool {
                                          return pair.first !=
                                                 (img_mode == ImageMode::X64 ? R_X86_64_RELATIVE : R_386_RELATIVE); // NOLINT(bugprone-branch-clone)
                                      })) {
                assert(reloc->info_raw.has_value());
                *out_ptr = Rela{
                    .r_offset = reloc->rva.template as<decltype(Rela::r_offset)>(),
                    .r_info = static_cast<decltype(Rela::r_info)>(*reloc->info_raw),
                    .r_addend = static_cast<decltype(Rela::r_addend)>(reloc->addend.value_or(0)),
                };
                ++out_ptr;
            }
        };
        switch (img_mode) {
        case ImageMode::X64: {
            assemble_structs.operator()<Elf64_Rela>();
            break;
        }
        case ImageMode::X86: {
            assemble_structs.operator()<Elf32_Rela>();
            break;
        }
        default:
            throw std::out_of_range("cont::elf::detail::update_relocations: unsupported image mode");
        }

        /// Update the rela entry
        assert(rela != nullptr && rela_sz != nullptr);
        rela->value = sec.virtual_address;
        rela_sz->value = sec_hdr.size_raw_data;
        logger::debug("elf: updated relocations: {} entries (now at {:#x})", image->relocations.size(), rela->value);
    }
} // namespace cont::elf::detail
