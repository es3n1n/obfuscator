#include "cont/elf/rebuilder/rebuilder.hpp"

namespace cont::elf::detail {
    void init_header(Image* image, std::vector<std::uint8_t>& result) {
        const auto img_mode = image->mode();

        /// We will be rewriting PHT entries, let's alloc a segment for that
        auto pht_sec_hdr = Section{};
        pht_sec_hdr.name = "pht";
        pht_sec_hdr.size_raw_data = (img_mode == ImageMode::X64 ? sizeof(Elf64_Phdr) : sizeof(Elf32_Phdr)) * image->sections.size();
        pht_sec_hdr.characteristics.mem_read = true;
        pht_sec_hdr.elf_type = PT_LOAD;
        auto& pht_seg = image->new_section(pht_sec_hdr);

        /// Update phdr segment
        auto& old_pht_seg =
            image->find_section_if([](const Section& section) -> bool { return section.elf_type.has_value() && *section.elf_type == PT_PHDR; });
        old_pht_seg.ptr_raw_data = pht_seg.ptr_raw_data;
        old_pht_seg.size_raw_data = pht_seg.size_raw_data;
        old_pht_seg.virtual_address = pht_seg.virtual_address;
        old_pht_seg.virtual_size = pht_seg.virtual_size;

        /// Serializing PHT entries
        const auto serialize_pht = [&image, &pht_seg]<typename ElfPhdr>() -> void {
            auto* hdr = memory::address(pht_seg.raw_data.data()).ptr<ElfPhdr>();

            for (auto& section : image->sections) {
                if (section.symbolic) {
                    continue;
                }
                const auto elf_type = section.elf_type.value_or(PT_LOAD);
                const auto elf_alignment = section.elf_alignment.value_or(image->get_section_alignment());

                decltype(ElfPhdr::p_flags) flags = 0;
                if (section.characteristics.mem_read) {
                    flags |= PF_R;
                }
                if (section.characteristics.mem_write) {
                    flags |= PF_W;
                }
                if (section.characteristics.mem_execute) {
                    flags |= PF_X;
                }

                hdr->p_type = static_cast<decltype(ElfPhdr::p_type)>(elf_type);
                hdr->p_flags = flags;
                hdr->p_offset = static_cast<decltype(ElfPhdr::p_offset)>(section.ptr_raw_data);
                hdr->p_vaddr = static_cast<decltype(ElfPhdr::p_vaddr)>(section.virtual_address);
                hdr->p_paddr = static_cast<decltype(ElfPhdr::p_paddr)>(section.virtual_address);
                hdr->p_filesz = static_cast<decltype(ElfPhdr::p_filesz)>(section.size_raw_data);
                hdr->p_memsz = static_cast<decltype(ElfPhdr::p_memsz)>(section.virtual_size);
                hdr->p_align = static_cast<decltype(ElfPhdr::p_align)>(elf_alignment);
                ++hdr;
            }
        };

        /// Preparing header, it will be copied with all segments later
        const auto prepare_header = [&result, &image, &pht_seg]<typename Ehdr>() -> void {
            const auto& last_sec = image->find_last_section();
            result.resize(last_sec.ptr_raw_data + last_sec.size_raw_data);

            auto* original_ehdr = image->rva_to_ptr<Ehdr>(nullptr);

            /// Update PHT file offset
            original_ehdr->e_phoff = static_cast<decltype(Ehdr::e_phoff)>(pht_seg.ptr_raw_data);
            original_ehdr->e_phnum = static_cast<decltype(Ehdr::e_phnum)>(image->sections.size());
        };

        /// Updating .dynamic
        const auto update_dynamic = [&image]<typename EDyn>() -> void {
            auto* dyn = image->find_segment_with_type(PT_DYNAMIC);
            if (dyn == nullptr) {
                throw std::runtime_error("no dynamic?");
            }

            auto* edyn = memory::address(dyn->raw_data.data()).ptr<EDyn>();
            for (const auto& dynamic : image->dynamics) {
                edyn->d_tag = static_cast<decltype(EDyn::d_tag)>(dynamic.tag);
                edyn->d_un.d_val = static_cast<decltype(EDyn::d_un.d_val)>(dynamic.value);
                ++edyn;
            }

            edyn->d_tag = DT_NULL;
            edyn->d_un.d_val = DT_NULL;
        };

        switch (img_mode) {
        case ImageMode::X64: {
            serialize_pht.operator()<Elf64_Phdr>();
            update_dynamic.operator()<Elf64_Dyn>();
            prepare_header.operator()<Elf64_Ehdr>();
            break;
        }
        case ImageMode::X86: {
            serialize_pht.operator()<Elf32_Phdr>();
            update_dynamic.operator()<Elf32_Dyn>();
            prepare_header.operator()<Elf32_Ehdr>();
            break;
        }
        default:
            throw std::out_of_range("cont::elf::detail::init_header: Unsupported image mode");
        }
    }
} // namespace cont::elf::detail
