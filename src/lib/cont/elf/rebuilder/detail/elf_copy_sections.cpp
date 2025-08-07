#include "cont/elf/rebuilder/rebuilder.hpp"

#include <algorithm>
#include <cstring>

namespace cont::elf::detail {
    constexpr std::uint64_t make_pflags(const Section& s) {
        std::uint64_t f = 0;
        if (s.characteristics.mem_read) {
            f |= PF_R;
        }
        if (s.characteristics.mem_write) {
            f |= PF_W;
        }
        if (s.characteristics.mem_execute) {
            f |= PF_X;
        }
        return f;
    }

    template <typename Eh, typename Ph>
    void copy_sections_impl(Image* image, std::vector<std::uint8_t>& out) {
        const std::size_t file_align = image->get_file_alignment();
        const std::size_t sect_align = image->get_section_alignment();
        const std::size_t ph_align = (sizeof(Ph) == 56) ? 8 : 4;
        image->realign_sections();

        if (out.size() < sizeof(Eh)) {
            throw std::runtime_error("elf::copy_sections: header not copied yet");
        }
        out.resize(sizeof(Eh));

        std::vector<Ph> phdrs;
        phdrs.reserve(image->sections.size());

        for (auto& sec : image->sections) {
            const std::size_t off = memory::address(out.size()).align_up(file_align).inner();
            sec.ptr_raw_data = off;

            out.resize(off, 0);
            if (!sec.raw_data.empty()) {
                out.insert(out.end(), sec.raw_data.begin(), sec.raw_data.end());
            }

            sec.size_raw_data = static_cast<std::uint32_t>(sec.raw_data.size());

            Ph ph{};
            ph.p_type = PT_LOAD;
            ph.p_offset = static_cast<decltype(ph.p_offset)>(sec.ptr_raw_data);
            ph.p_vaddr = static_cast<decltype(ph.p_vaddr)>(sec.virtual_address);
            ph.p_paddr = ph.p_vaddr;
            ph.p_filesz = static_cast<decltype(ph.p_filesz)>(sec.size_raw_data);
            ph.p_memsz = static_cast<decltype(ph.p_memsz)>(sec.virtual_size);
            ph.p_flags = static_cast<decltype(ph.p_flags)>(make_pflags(sec));
            ph.p_align = static_cast<decltype(ph.p_align)>(sect_align);
            phdrs.push_back(ph);
        }

        const std::size_t new_phoff = memory::address(out.size()).align_up(ph_align).inner();
        out.resize(new_phoff, 0);
        const auto* src = reinterpret_cast<std::uint8_t*>(phdrs.data());
        out.insert(out.end(), src, src + phdrs.size() * sizeof(Ph));

        Eh* ehdr_new = reinterpret_cast<Eh*>(out.data());
        ehdr_new->e_phoff = static_cast<decltype(ehdr_new->e_phoff)>(new_phoff);
        ehdr_new->e_phentsize = sizeof(Ph);
        ehdr_new->e_phnum = static_cast<std::uint16_t>(phdrs.size());
    }

    void copy_sections(Image* image, std::vector<std::uint8_t>& out) {
        switch (image->mode()) {
        case ImageMode::X64:
            copy_sections_impl<Elf64_Ehdr, Elf64_Phdr>(image, out);
            break;

        case ImageMode::X86:
            copy_sections_impl<Elf32_Ehdr, Elf32_Phdr>(image, out);
            break;

        default:
            throw std::out_of_range("cont::elf::detail::copy_sections: unsupported image mode");
        }
    }
} // namespace cont::elf::detail