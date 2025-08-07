#include "cont/elf/rebuilder/rebuilder.hpp"

#include <algorithm>
#include <linux/elf.h>
#include <vector>

#define ELF64_R_INFO(sym, type) ((((std::uint64_t)(sym)) << 32) | ((std::uint64_t)(type)))

namespace cont::elf::detail {
    constexpr std::uint32_t make_rtype(const Relocation& r, ImageMode m) {
        return (r.type == RelocationType::Dir64) ? R_X86_64_RELATIVE : R_X86_64_GLOB_DAT;
    }

    template <typename Eh = Elf64_Ehdr, typename Ph = Elf64_Phdr>
    void write_relocations_x64(Image* img, std::vector<std::uint8_t>& file) {
        std::vector<Elf64_Rela> rela;
        rela.reserve(img->relocations.size());

        for (const auto& [rva, r] : img->relocations) {
            Elf64_Rela e{};
            e.r_offset = rva.inner();
            e.r_info = ELF64_R_INFO(r.sym_index.value_or(0), make_rtype(r, ImageMode::X64));
            e.r_addend = r.addend.value_or(0);
            rela.push_back(e);
        }

        std::ranges::sort(rela, {}, &Elf64_Rela::r_offset);

        Eh* eh = reinterpret_cast<Eh*>(file.data());
        auto* ph_first = reinterpret_cast<Ph*>(file.data() + eh->e_phoff);

        auto file2va = [&](std::size_t off) -> std::size_t {
            const Ph* best = nullptr;

            for (std::size_t i = 0; i < eh->e_phnum; ++i) {
                const Ph& ph = ph_first[i];
                if (ph.p_type != PT_LOAD)
                    continue;
                if (off >= ph.p_offset)
                    if (!best || ph.p_offset > best->p_offset)
                        best = &ph;
            }

            if (!best)
                throw std::runtime_error("elf::write_relocations: no PT_LOAD precedes offset");

            return best->p_vaddr + (off - best->p_offset);
        };

        const std::size_t rel_off = memory::address(file.size()).align_up(8).inner();
        const std::size_t rel_addr = file2va(rel_off);
        const std::size_t rel_sz = rela.size() * sizeof(Elf64_Rela);
        file.resize(rel_off, 0);
        file.insert(file.end(), reinterpret_cast<const std::uint8_t*>(rela.data()), reinterpret_cast<const std::uint8_t*>(rela.data()) + rel_sz);

        const Elf64_Dyn* old_dyn = nullptr;

        for (std::size_t i = 0; i < eh->e_phnum && !old_dyn; ++i) {
            if (ph_first[i].p_type == PT_DYNAMIC) {
                old_dyn = reinterpret_cast<const Elf64_Dyn*>(file.data() + ph_first[i].p_offset);
            }
        }

        if (!old_dyn) {
            const Elf64_Ehdr* orig_eh = img->x64();
            const auto orig_ph = static_cast<const Elf64_Phdr*>(img->raw_image().offset(orig_eh->e_phoff).as<const void*>());

            for (std::size_t i = 0; i < orig_eh->e_phnum; ++i)
                if (orig_ph[i].p_type == PT_DYNAMIC) {
                    old_dyn = img->raw_image().offset(orig_ph[i].p_offset).as<const Elf64_Dyn*>();
                    break;
                }
        }

        if (!old_dyn) {
            throw std::runtime_error("write_relocations: PT_DYNAMIC not found in rebuilt or original file");
        }

        std::vector<Elf64_Dyn> dyn_original;
        for (const Elf64_Dyn* d = old_dyn; d->d_tag != DT_NULL; ++d) {
            dyn_original.push_back(*d);
        }

        auto is_old_reloc_tag = [](std::uint64_t tag) {
            return tag == DT_RELA || tag == DT_RELASZ || tag == DT_RELAENT || tag == DT_REL || tag == DT_RELSZ || tag == DT_RELENT || tag == DT_RELCOUNT ||
                   tag == DT_RELACOUNT;
        };

        std::vector<Elf64_Dyn> dyn_new;
        auto push = [&](std::uint64_t t, std::uint64_t v) {
            Elf64_Dyn d{};
            d.d_tag = t;
            d.d_un.d_val = v;
            dyn_new.push_back(d);
        };

        for (const Elf64_Dyn& d : dyn_original) {
            if (!is_old_reloc_tag(d.d_tag)) {
                dyn_new.push_back(d);
            }
        }

        push(DT_RELA, rel_addr);
        push(DT_RELASZ, rel_sz);
        push(DT_RELAENT, sizeof(Elf64_Rela));
        push(DT_NULL, 0);

        const std::size_t dyn_off = memory::address(file.size()).align_up(8).inner();
        const std::size_t dyn_sz = dyn_new.size() * sizeof(Elf64_Dyn);
        const std::size_t dyn_addr = file2va(dyn_off);

        file.resize(dyn_off, 0);
        file.insert(file.end(), reinterpret_cast<const std::uint8_t*>(dyn_new.data()), reinterpret_cast<const std::uint8_t*>(dyn_new.data()) + dyn_sz);

        std::vector<Ph> phdrs(reinterpret_cast<Ph*>(file.data() + eh->e_phoff), reinterpret_cast<Ph*>(file.data() + eh->e_phoff) + eh->e_phnum);
        const std::size_t dyn_end = dyn_off + dyn_sz;

        Ph* load_to_grow = nullptr;
        for (auto& ph : phdrs) {
            if (ph.p_type == PT_LOAD && ph.p_offset <= dyn_off) {
                if (!load_to_grow || ph.p_offset > load_to_grow->p_offset) {
                    load_to_grow = &ph;
                }
            }
        }

        if (load_to_grow == nullptr) {
            throw std::runtime_error("elf::write_relocations: no PT_LOAD precedes .dynamic");
        }

        const std::size_t cur_end = load_to_grow->p_offset + load_to_grow->p_filesz;
        if (dyn_end > cur_end) {
            const std::size_t delta = dyn_end - cur_end;
            load_to_grow->p_filesz += delta;
            load_to_grow->p_memsz += delta;
            load_to_grow->p_flags |= PF_R | PF_W;
        }

        Ph dyn_seg{};
        dyn_seg.p_type = PT_DYNAMIC;
        dyn_seg.p_offset = dyn_off;
        dyn_seg.p_vaddr = dyn_addr;
        dyn_seg.p_paddr = dyn_seg.p_vaddr;
        dyn_seg.p_filesz = dyn_sz;
        dyn_seg.p_memsz = dyn_sz;
        dyn_seg.p_flags = PF_R | PF_W;
        dyn_seg.p_align = 8;
        phdrs.push_back(dyn_seg);

        const std::size_t new_phoff = memory::address(file.size()).align_up(8).inner();
        file.resize(new_phoff, 0);
        file.insert(file.end(), reinterpret_cast<const std::uint8_t*>(phdrs.data()),
                    reinterpret_cast<const std::uint8_t*>(phdrs.data()) + phdrs.size() * sizeof(Ph));

        eh->e_phoff = new_phoff;
        eh->e_phnum = static_cast<std::uint16_t>(phdrs.size());
        eh->e_phentsize = sizeof(Ph);
    }

    void write_relocations(Image* img, std::vector<std::uint8_t>& file) {
        switch (img->mode()) {
        case ImageMode::X64:
            write_relocations_x64(img, file);
            break;
        default:
            throw std::out_of_range("elf::write_relocations: bad mode");
        }
    }
} // namespace cont::elf::detail

#undef ELF64_R_INFO
