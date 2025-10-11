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
        return std::memcmp(static_cast<uint8_t*>(x64()->e_ident), ELFMAG, SELFMAG) == 0;
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
        const auto ctx = RebuilderContext{.image = this};
        return rebuild_elf(ctx);
    }

    void Image::update_sections() {
        auto build_from_segments = [&]<typename ElfEhdr, typename ElfPhdr, typename ElfDyn>(const ElfEhdr* ehdr) -> void {
            auto phdrs = phdr_span<ElfEhdr, ElfPhdr>(ehdr);

            sections.clear();
            for (const ElfPhdr& phdr : phdrs) {
                auto& sec = sections.emplace_back(to_cont(phdr));

                sec.raw_data.resize(sec.size_raw_data, 0);
                if (sec.size_raw_data != 0) {
                    const auto* src = raw_image_.offset(sec.ptr_raw_data).template as<const std::uint8_t*>();
                    std::memcpy(sec.raw_data.data(), src, sec.size_raw_data);
                }

                if (sec.elf_type == PT_DYNAMIC) {
                    dynamics.clear();
                    for (auto* iter = memory::address(sec.raw_data.data()).ptr<ElfDyn>(); iter != nullptr && iter->d_tag != DT_NULL; ++iter) {
                        auto& [tag, value] = dynamics.emplace_back();
                        tag = iter->d_tag;
                        value = iter->d_un.d_val;
                    }
                }
            }

            if (const auto* const plt_got = find_dynamic(DT_PLTGOT); plt_got != nullptr) {
                sections.emplace_back(Section{
                    .name = ".plt",
                    .virtual_size = 0x1000,
                    .virtual_address = plt_got->value,
                    .size_raw_data = 0,
                    .ptr_raw_data = 0,
                    .symbolic = true,
                    .elf_type = PT_DYNAMIC,
                });
            }

            std::ranges::sort(sections, [](const Section& lhs, const Section& rhs) -> bool {
                /// PHDR should always be first
                if (lhs.elf_type.value() == PT_PHDR) {
                    return true;
                }
                if (rhs.elf_type.value() == PT_PHDR) {
                    return false;
                }
                return lhs.virtual_address < rhs.virtual_address;
            });
        };

        switch (mode()) {
        case ImageMode::X64: {
            const auto* const ehdr = x64();
            build_from_segments.operator()<Elf64_Ehdr, Elf64_Phdr, Elf64_Dyn>(ehdr);
            break;
        }
        case ImageMode::X86: {
            const auto* const ehdr = x86();
            build_from_segments.operator()<Elf32_Ehdr, Elf32_Phdr, Elf32_Dyn>(ehdr);
            break;
        }
        default:
            throw std::out_of_range("cont::elf::update_sections: unsupported image mode");
        }

        logger::debug("elf: parsed {} sections", sections.size());
    }

    void Image::update_relocations() {
        const auto img_mode = mode();

        const auto proceed = [this, img_mode]<typename Rela>() -> void {
            if (const auto* const rela = find_dynamic(DT_RELA); rela != nullptr) {
                const auto* const rela_sz = find_dynamic(DT_RELASZ);
                if (rela_sz == nullptr || rela->value == 0) {
                    throw std::runtime_error("cont::elf: DT_RELA or DT_RELASZ not found or invalid");
                }

                const auto* rela_ptr = rva_to_ptr<const Rela>(rela->value);
                for (const auto* const rela_end = rela_ptr + (rela_sz->value / sizeof(Rela)); rela_ptr < rela_end; ++rela_ptr) {
                    const auto converted_type = to_cont(rela_ptr->r_info, img_mode);
                    relocations[rela_ptr->r_offset] =
                        Relocation{.rva = rela_ptr->r_offset, .type = converted_type, .addend = rela_ptr->r_addend, .info_raw = rela_ptr->r_info};
                }
            }

            if (const auto* const jmprel = find_dynamic(DT_JMPREL); jmprel != nullptr) {
                const auto* rela_ptr = raw_image_.offset(static_cast<std::ptrdiff_t>(jmprel->value)).as<const Rela*>();
                for (; rela_ptr->r_info != 0; ++rela_ptr) {
                    const auto converted_type = to_cont(rela_ptr->r_info, img_mode);
                    jmprel_relocations[rela_ptr->r_offset] =
                        Relocation{.rva = rela_ptr->r_offset, .type = converted_type, .addend = rela_ptr->r_addend, .info_raw = rela_ptr->r_info};
                }
            }
        };

        switch (img_mode) {
        case ImageMode::X64: {
            proceed.operator()<Elf64_Rela>();
            break;
        }
        case ImageMode::X86: {
            proceed.operator()<Elf32_Rela>();
            break;
        }
        default:
            throw std::out_of_range("cont::elf::update_relocations: unsupported image mode");
        }

        logger::debug("elf: parsed {}(+{}) relocations", relocations.size(), jmprel_relocations.size());
    }
} // namespace cont::elf
