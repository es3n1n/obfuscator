#pragma once
#include "cont/elf/rebuilder/rebuilder.hpp"

#include <algorithm>
#include <vector>

namespace cont::elf::detail {
    static constexpr std::uint64_t make_sh_flags(const Section& s) {
        std::uint64_t f = 0;
        if (s.characteristics.mem_read) {
            f |= SHF_ALLOC;
        }
        if (s.characteristics.mem_write) {
            f |= SHF_WRITE;
        }
        if (s.characteristics.mem_execute) {
            f |= SHF_EXECINSTR;
        }
        return f;
    }

    template <typename Eh, typename Sh>
    void build_section_headers_impl(Image* image, std::vector<std::uint8_t>& file) {
        std::vector<char> strtab;
        strtab.push_back('\0');

        auto push_name = [&strtab](const std::string_view& name) -> std::uint32_t {
            const std::uint32_t off = static_cast<std::uint32_t>(strtab.size());
            strtab.insert(strtab.end(), name.begin(), name.end());
            strtab.push_back('\0');
            return off;
        };

        std::vector<std::uint32_t> name_indices;
        name_indices.reserve(image->sections.size());

        for (const auto& s : image->sections)
            name_indices.push_back(push_name(s.name.value_or("anon")));

        const std::uint32_t shstrtab_name_off = push_name(".shstrtab");
        const std::size_t shdr_count = 1 + image->sections.size() + 1;
        std::vector<Sh> shdrs(shdr_count);
        const std::size_t sec_align = image->get_section_alignment();

        for (std::size_t i = 0; i < image->sections.size(); ++i) {
            const Section& s = image->sections[i];
            Sh& hdr = shdrs[1 + i];

            hdr.sh_name = name_indices[i];
            hdr.sh_type = s.raw_data.empty() ? SHT_NOBITS : SHT_PROGBITS;
            hdr.sh_flags = make_sh_flags(s);
            hdr.sh_addr = s.virtual_address;
            hdr.sh_offset = s.raw_data.empty() ? 0 : s.ptr_raw_data;
            hdr.sh_size = s.raw_data.empty() ? s.virtual_size : static_cast<std::uint64_t>(s.raw_data.size());
            hdr.sh_addralign = sec_align;
            hdr.sh_entsize = 0;
            hdr.sh_link = 0;
            hdr.sh_info = 0;
        }

        Sh& shstr_hdr = shdrs.back();
        shstr_hdr.sh_name = shstrtab_name_off;
        shstr_hdr.sh_type = SHT_STRTAB;
        shstr_hdr.sh_flags = 0;
        shstr_hdr.sh_addr = 0;
        shstr_hdr.sh_offset = 0;
        shstr_hdr.sh_size = static_cast<std::uint64_t>(strtab.size());
        shstr_hdr.sh_addralign = 1;
        shstr_hdr.sh_entsize = 0;

        const std::size_t shstrtab_off = file.size();
        file.insert(file.end(), strtab.begin(), strtab.end());
        shstr_hdr.sh_offset = shstrtab_off;

        const std::size_t shdr_align = (sizeof(Sh) == 64) ? 8 : 4;
        const std::size_t aligned = (file.size() + shdr_align - 1) & ~(shdr_align - 1);
        file.resize(aligned, 0);

        const std::size_t shdr_off = file.size();
        file.insert(file.end(), reinterpret_cast<std::uint8_t*>(shdrs.data()), reinterpret_cast<std::uint8_t*>(shdrs.data()) + shdrs.size() * sizeof(Sh));

        Eh* eh = reinterpret_cast<Eh*>(file.data());
        eh->e_shoff = shdr_off;
        eh->e_shnum = static_cast<std::uint16_t>(shdr_count);
        eh->e_shstrndx = static_cast<std::uint16_t>(shdr_count - 1);
    }

    void build_section_headers(Image* img, std::vector<std::uint8_t>& file) {
        switch (img->mode()) {
        case ImageMode::X64:
            build_section_headers_impl<Elf64_Ehdr, Elf64_Shdr>(img, file);
            break;
        case ImageMode::X86:
            build_section_headers_impl<Elf32_Ehdr, Elf32_Shdr>(img, file);
            break;
        default:
            throw std::out_of_range("elf::build_section_headers: bad mode");
        }
    }
} // namespace cont::elf::detail