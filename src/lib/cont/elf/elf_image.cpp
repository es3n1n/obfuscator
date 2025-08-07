#include "cont/elf/image.hpp"
#include "cont/elf/rebuilder/rebuilder.hpp"

#include <algorithm>
#include <es3n1n/common/logger.hpp>

namespace cont::elf {
    [[nodiscard]] ImageMode Image::mode() const {
        switch (x64()->e_ident[EI_CLASS]) {
        case ELFCLASS64:
            return ImageMode::X64;
        case ELFCLASS32:
            return ImageMode::X86;
        default:
            throw std::runtime_error("cont::elf: Unsupported ELF class");
        }
    }

    [[nodiscard]] bool Image::verify_integrity() const {
        return std::memcmp(x64()->e_ident, ELFMAG, SELFMAG) == 0;
    }

    [[nodiscard]] std::size_t Image::get_image_base() const {
        static auto result = [this]() -> std::size_t {
            switch (mode()) {
            case ImageMode::X64: {
                auto phdrs = phdr_span<Elf64_Ehdr, Elf64_Phdr>(x64());
                const auto it = std::ranges::find_if(phdrs, [](const Elf64_Phdr& ph) { return ph.p_type == PT_LOAD; });
                if (it == phdrs.end()) {
                    throw std::runtime_error("cont::elf: No PT_LOAD segment found");
                }

                return it->p_vaddr;
            }

            case ImageMode::X86: {
                auto phdrs = phdr_span<Elf32_Ehdr, Elf32_Phdr>(x86());
                const auto it = std::ranges::find_if(phdrs, [](const Elf32_Phdr& ph) { return ph.p_type == PT_LOAD; });
                if (it == phdrs.end()) {
                    throw std::runtime_error("cont::elf: No PT_LOAD segment found");
                }

                return it->p_vaddr;
            }
            default:
                throw std::runtime_error("cont::elf: Unsupported ELF mode");
            }
        }();
        return result;
    }

    [[nodiscard]] std::size_t Image::get_section_alignment() const {
        static auto result = [this]() -> std::size_t {
            switch (mode()) {
            case ImageMode::X64: {
                auto phdrs = phdr_span<Elf64_Ehdr, Elf64_Phdr>(x64());
                const auto it = std::ranges::find_if(phdrs, [](const Elf64_Phdr& ph) { return ph.p_type == PT_LOAD; });
                if (it == phdrs.end()) {
                    throw std::runtime_error("cont::elf: No PT_LOAD segment found");
                }

                return it->p_align;
            }

            case ImageMode::X86: {
                auto phdrs = phdr_span<Elf32_Ehdr, Elf32_Phdr>(x86());
                const auto it = std::ranges::find_if(phdrs, [](const Elf32_Phdr& ph) { return ph.p_type == PT_LOAD; });
                if (it == phdrs.end()) {
                    throw std::runtime_error("cont::elf: No PT_LOAD segment found");
                }

                return it->p_align;
            }
            default:
                throw std::runtime_error("cont::elf: Unsupported ELF mode");
            }
        }();
        return result;
    }

    [[nodiscard]] std::size_t Image::get_file_alignment() const {
        return get_section_alignment();
    }

    [[nodiscard]] std::vector<std::uint8_t> Image::rebuild_image() {
        auto ctx = RebuilderContext{.image = this};
        return rebuild_elf(ctx);
    }

    void Image::update_sections() {
        auto build_from_segments = [this]<typename ElfEhdr, typename ElfPhdr>(const ElfEhdr* ehdr) -> void {
            auto phdrs = phdr_span<ElfEhdr, ElfPhdr>(ehdr);

            sections.clear();
            for (const ElfPhdr& phdr : phdrs) {
                if (phdr.p_type != PT_LOAD) {
                    continue;
                }

                auto& sec = sections.emplace_back(to_cont(phdr));

                sec.raw_data.resize(sec.size_raw_data, 0);
                if (sec.size_raw_data != 0) {
                    const auto* src = raw_image_.offset(sec.ptr_raw_data).as<const std::uint8_t*>();
                    std::memcpy(sec.raw_data.data(), src, sec.size_raw_data);
                }
            }

            std::ranges::sort(sections, {}, &cont::Section::virtual_address);
        };

        switch (mode()) {
        case ImageMode::X64: {
            const auto ehdr = x64();
            build_from_segments.operator()<Elf64_Ehdr, Elf64_Phdr>(ehdr);
            break;
        }
        case ImageMode::X86: {
            const auto ehdr = x86();
            build_from_segments.operator()<Elf32_Ehdr, Elf32_Phdr>(ehdr);
            break;
        }
        default:
            throw std::out_of_range("cont::elf::update_sections: unsupported image mode");
        }

        logger::debug("elf: parsed {} sections", sections.size());
    }

    void Image::update_relocations() {
        const auto img_mode = mode();

        const auto parse_rel_section = [this, img_mode](const auto* rel, const std::size_t count, const std::size_t elem_size) {
            using RelT = std::remove_cvref_t<decltype(*rel)>;

            for (std::size_t i = 0; i < count; ++i) {
                const RelT& ent = rel[i];

                const std::uint32_t r_type = (img_mode == ImageMode::X64) ? ELF64_R_TYPE(ent.r_info) : ELF32_R_TYPE(ent.r_info);

                const auto type = to_cont(r_type, img_mode);
                auto rva = static_cast<memory::address>(ent.r_offset);
                if (relocations.contains(rva)) {
                    throw std::runtime_error(std::format("elf: duplicated reloc rva {:#x}", rva));
                }

                Relocation reloc;
                reloc.rva = rva;
                reloc.size = static_cast<std::uint8_t>(elem_size);
                reloc.type = type;
                if constexpr (traits::is_any_of_v<RelT, Elf64_Rela, Elf32_Rela>) {
                    reloc.addend = ent.r_addend;
                } else {
                    if (elem_size == 8) {
                        reloc.addend = *raw_image_.offset(ent.r_offset).as<const std::int64_t*>();
                    }
                    assert(elem_size == 4);
                    reloc.addend = *raw_image_.offset(ent.r_offset).as<const std::int32_t*>();
                }

                if (reloc.type == RelocationType::GlobDat32 || reloc.type == RelocationType::GlobDat64) {
                    reloc.sym_index = img_mode == ImageMode::X64 ? ELF64_R_SYM(ent.r_info) : ELF32_R_SYM(ent.r_info);
                }

                relocations.emplace(rva, reloc);
            }
        };

        const auto proceed = [this, parse_rel_section]<typename Eh, typename Phdr, typename Rel, typename Rela>(const Eh* ehdr,
                                                                                                                const std::size_t size) -> void {
            auto phdrs = phdr_span<Eh, Phdr>(ehdr);
            const Elf64_Dyn* dyn = nullptr;
            std::size_t dyn_cnt = 0;
            std::size_t dyn_segments = 0;

            for (const auto& ph : phdrs) {
                if (ph.p_type != PT_DYNAMIC) {
                    continue;
                }

                dyn = raw_image_.offset(ph.p_offset).as<const Elf64_Dyn*>();
                dyn_cnt = ph.p_filesz / sizeof(Elf64_Dyn);
                dyn_segments++;
            }

            if (dyn_segments != 1) {
                throw std::runtime_error("cont::elf: expected exactly one PT_DYNAMIC segment");
            }

            Elf64_Addr rel = 0, rela = 0;
            std::size_t relsz = 0, relasz = 0;

            for (std::size_t i = 0; i < dyn_cnt; ++i) {
                switch (dyn[i].d_tag) {
                case DT_REL:
                    rel = dyn[i].d_un.d_val;
                    break;
                case DT_RELSZ:
                    relsz = dyn[i].d_un.d_val;
                    break;
                case DT_RELA:
                    rela = dyn[i].d_un.d_val;
                    break;
                case DT_RELASZ:
                    relasz = dyn[i].d_un.d_val;
                    break;
                default:
                    break;
                }
            }

            if (rel && relsz) {
                auto* p = raw_image_.offset(rel).as<const Rel*>();
                parse_rel_section(p, relsz / sizeof(Rel), size);
            }
            if (rela && relasz) {
                auto* p = raw_image_.offset(rela).as<const Rela*>();
                parse_rel_section(p, relasz / sizeof(Rela), size);
            }
        };

        switch (img_mode) {
        case ImageMode::X64: {
            const auto ehdr = x64();
            proceed.operator()<Elf64_Ehdr, Elf64_Phdr, Elf64_Rel, Elf64_Rela>(ehdr, sizeof(std::uint64_t));
            break;
        }
        case ImageMode::X86: {
            const auto ehdr = x86();
            proceed.operator()<Elf32_Ehdr, Elf32_Phdr, Elf64_Rel, Elf64_Rela>(ehdr, sizeof(std::uint32_t));
            break;
        }
        default:
            throw std::out_of_range("cont::elf::update_relocations: unsupported image mode");
        }

        logger::debug("elf: parsed {} relocations", relocations.size());
    }
} // namespace cont::elf
