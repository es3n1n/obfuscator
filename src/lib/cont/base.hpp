#pragma once
#include <coff/section_header.hpp>
#include <linux/elf.h>
#include <nt/directories/dir_relocs.hpp>
#include <zasm/base/mode.hpp>

#include "es3n1n/common/memory/address.hpp"
#include "util/structs.hpp"

#include <array>
#include <cassert>
#include <string>

namespace cont {
    enum struct ContImageType : std::uint8_t {
        PE = 0,
        ELF = 1,
    };

    enum struct ImageMode : std::uint8_t {
        X64 = 0,
        X86 = 1,
    };

    enum struct RelocationType : std::uint8_t {
        Absolute = 0,
        High = 1,
        Low = 2,
        HighLow = 3,
        HighAdj = 4,
        Ia64Imm64 = 5,
        Dir64 = 6,
        GlobDat64 = 7,
        GlobDat32 = 7,
    };

    struct Relocation {
        memory::address rva;
        std::uint8_t size = 0; // in bytes
        RelocationType type;
        std::optional<std::size_t> sym_index = std::nullopt;
        std::optional<std::ptrdiff_t> addend = std::nullopt;
    };

    enum struct DirectoryType : std::uint8_t {
        Export = 0,
        Import = 1,
        Resource = 2,
        Exception = 3,
        Security = 4,
        Reloc = 5,
        Debug = 6,
        Copyright = 7,
        Architecture = 8,
        GlobalPtr = 9,
        Tls = 10,
        LoadConfig = 11,
        BoundImport = 12,
        Iat = 13,
        DelayImport = 14,
        ComDescriptor = 15,
        MAX_LENGTH = 16,
    };

    struct DirectoryProperties {
        std::size_t offset; // from the section start
        std::size_t size; // in bytes
    };

    struct Section {
        std::optional<std::string> name = std::nullopt;
        std::size_t virtual_size = 0U;
        std::size_t virtual_address = 0U;
        std::size_t size_raw_data = 0U;
        std::size_t ptr_raw_data = 0U;

        std::vector<std::uint8_t> raw_data;
        std::array<std::optional<DirectoryProperties>, std::to_underlying(DirectoryType::MAX_LENGTH)> contains_directories = {};

        union Characteristics {
            uint32_t flags;
            struct {
                uint32_t cnt_code:1; // Section contains code.
                uint32_t cnt_init_data:1; // Section contains initialized data.
                uint32_t cnt_uninit_data:1; // Section contains uninitialized data.
                uint32_t lnk_info:1; // Section contains comments or some other type of information.
                uint32_t lnk_remove:1; // Section contents will not become part of image.
                uint32_t lnk_comdat:1; // Section contents comdat.
                uint32_t no_defer_spec_exc:1; // Reset speculative exceptions handling bits in the TLB entries for this section.
                uint32_t mem_far:1;
                uint32_t mem_purgeable:1;
                uint32_t mem_locked:1;
                uint32_t mem_preload:1;
                uint32_t alignment:4; // Alignment calculated as: n ? 1 << ( n - 1 ) : 16
                uint32_t lnk_nreloc_ovfl:1; // Section contains extended relocations.
                uint32_t mem_discardable:1; // Section can be discarded.
                uint32_t mem_not_cached:1; // Section is not cachable.
                uint32_t mem_not_paged:1; // Section is not pageable.
                uint32_t mem_shared:1; // Section is shareable.
                uint32_t mem_execute:1; // Section is executable.
                uint32_t mem_read:1; // Section is readable.
                uint32_t mem_write:1; // Section is writeable.
            };
        } characteristics = {};

        [[nodiscard]] std::optional<DirectoryProperties> directory_info(const DirectoryType dir_type) const {
            return contains_directories[std::to_underlying(dir_type)];
        }

        [[nodiscard]] bool has_directory(const DirectoryType type) const {
            return contains_directories[std::to_underlying(type)].has_value();
        }

        void set_contained_dir(const DirectoryType type, const std::size_t offset, const std::size_t size) {
            contains_directories[std::to_underlying(type)] = DirectoryProperties{.offset = offset, .size = size};
        }
    };

    class ImageBase {
    public:
        explicit ImageBase(const ContImageType image_type, const memory::address raw_image): image_type_(image_type), raw_image_(raw_image) { }
        virtual ~ImageBase() = default;
        DEFAULT_COPY(ImageBase);

        [[nodiscard]] virtual ImageMode mode() const = 0;
        [[nodiscard]] virtual bool verify_integrity() const = 0;

        [[nodiscard]] virtual std::size_t get_image_base() const = 0;
        [[nodiscard]] virtual std::size_t get_section_alignment() const = 0;
        [[nodiscard]] virtual std::size_t get_file_alignment() const = 0;

        [[nodiscard]] virtual std::vector<std::uint8_t> rebuild_image() = 0;

        void realign_sections() {
            /// Nothing to realign
            if (sections.size() <= 1) {
                return;
            }

            /// Making sure that all section virtual sizes are aligned
            for (std::size_t i = 0; i < sections.size() - 1; ++i) {
                auto& sec = sections.at(i);
                const auto& next_sec = sections.at(i + 1);
                sec.virtual_size = next_sec.virtual_address - sec.virtual_address;
            }
        }

        [[nodiscard]] Section& new_section(Section object) {
            const auto section_alignment = get_section_alignment();
            const auto file_alignment = get_file_alignment();
            const auto last_section = find_last_section();

            auto& new_sec = sections.emplace_back(object);
            assert(new_sec.size_raw_data > 0);
            new_sec.raw_data.resize(new_sec.size_raw_data);

            new_sec.virtual_size = new_sec.size_raw_data = memory::address{new_sec.size_raw_data} //
                                                               .align_up(section_alignment)
                                                               .as<uint32_t>();

            new_sec.virtual_address = memory::address{last_section.virtual_address + last_section.virtual_size} //
                                          .align_up(section_alignment)
                                          .as<uint32_t>();

            new_sec.ptr_raw_data = memory::address{last_section.ptr_raw_data + last_section.size_raw_data} //
                                       .align_up(file_alignment)
                                       .as<uint32_t>();

            return new_sec;
        }

        [[nodiscard]] Section& find_section_if(const std::function<bool(const Section&)>& pred) {
            const auto iter = std::ranges::find_if(sections, pred);
            if (iter == sections.end()) {
                throw std::runtime_error("cont: Unable to find section by predicate");
            }
            return *iter;
        }

        [[nodiscard]] std::vector<Section> find_sections_if(const std::function<bool(const Section&)>& pred) const {
            std::vector<Section> result;
            for (const auto& section : sections) {
                if (pred(section)) {
                    result.push_back(section);
                }
            }
            return result;
        }

        [[nodiscard]] Section& find_last_section() {
            const auto result = std::ranges::max_element(sections, [](const Section& lhs, const Section& rhs) -> bool { //
                return lhs.virtual_address < rhs.virtual_address;
            });

            if (result == std::end(sections)) {
                throw std::runtime_error("cont: Unable to find last section");
            }

            return *result;
        }

        [[nodiscard]] Section* rva_to_section(std::uint32_t rva) {
            const auto iter = std::ranges::find_if(sections, [rva](const Section& sec) -> bool { //
                return rva >= sec.virtual_address && rva <= (sec.virtual_address + sec.virtual_size);
            });

            if (iter == sections.end()) {
                return nullptr;
            }

            return &*iter;
        }

        template <typename Ty = std::uint8_t>
        [[nodiscard]] Ty* rva_to_ptr(const memory::address rva) {
            const auto* section = rva_to_section(rva.as<std::uint32_t>());
            if (section == nullptr) {
                return nullptr;
            }

            const auto offset = rva.inner() - section->virtual_address;
            return memory::address{section->raw_data.data()}.offset(offset).as<std::add_pointer_t<Ty>>();
        }

        [[nodiscard]] zasm::MachineMode machine_mode() const {
            switch (mode()) {
            case ImageMode::X64:
                return zasm::MachineMode::AMD64;
            case ImageMode::X86:
                return zasm::MachineMode::I386;
            default:
                throw std::out_of_range("cont::ImageBase::machine_mode: Unsupported image mode");
            }
        }

        [[nodiscard]] std::size_t ptr_size() const {
            switch (mode()) {
            case ImageMode::X64:
                return sizeof(std::uint64_t);
            case ImageMode::X86:
                return sizeof(std::uint32_t);
            default:
                throw std::out_of_range("cont::ImageBase::ptr_size: Unsupported image mode");
            }
        }

        [[nodiscard]] memory::address raw_image() const noexcept {
            return raw_image_;
        }

        [[nodiscard]] ContImageType image_type() const noexcept {
            return image_type_;
        }

    protected:
        virtual void update_sections() = 0;
        virtual void update_relocations() = 0;

        void initialize() {
            update_sections();
            update_relocations();
        }

        memory::address raw_image_;
        ContImageType image_type_;

    public:
        /// An unordered map that consists of {rva: reloc_info}
        std::unordered_map<memory::address, Relocation> relocations;
        std::vector<Section> sections;
    };
} // namespace cont

constexpr cont::RelocationType to_cont(const win::reloc_type_id type) {
    switch (type) {
    case win::reloc_type_id::rel_based_absolute:
        return cont::RelocationType::Absolute;
    case win::reloc_type_id::rel_based_high:
        return cont::RelocationType::High;
    case win::reloc_type_id::rel_based_low:
        return cont::RelocationType::Low;
    case win::reloc_type_id::rel_based_high_low:
        return cont::RelocationType::HighLow;
    case win::reloc_type_id::rel_based_high_adj:
        return cont::RelocationType::HighAdj;
    case win::reloc_type_id::rel_based_ia64_imm64:
        return cont::RelocationType::Ia64Imm64;
    case win::reloc_type_id::rel_based_dir64:
        return cont::RelocationType::Dir64;
    }
    throw std::runtime_error("pe: unsupported relocation type");
}

constexpr cont::DirectoryType to_cont(const win::directory_id type) {
    switch (type) {
    case win::directory_id::directory_entry_export:
        return cont::DirectoryType::Export;
    case win::directory_id::directory_entry_import:
        return cont::DirectoryType::Import;
    case win::directory_id::directory_entry_resource:
        return cont::DirectoryType::Resource;
    case win::directory_id::directory_entry_exception:
        return cont::DirectoryType::Exception;
    case win::directory_id::directory_entry_security:
        return cont::DirectoryType::Security;
    case win::directory_id::directory_entry_basereloc:
        return cont::DirectoryType::Reloc;
    case win::directory_id::directory_entry_debug:
        return cont::DirectoryType::Debug;
    // no copyright
    case win::directory_id::directory_entry_architecture:
        return cont::DirectoryType::Architecture;
    case win::directory_id::directory_entry_globalptr:
        return cont::DirectoryType::GlobalPtr;
    case win::directory_id::directory_entry_tls:
        return cont::DirectoryType::Tls;
    case win::directory_id::directory_entry_load_config:
        return cont::DirectoryType::LoadConfig;
    case win::directory_id::directory_entry_bound_import:
        return cont::DirectoryType::BoundImport;
    case win::directory_id::directory_entry_iat:
        return cont::DirectoryType::Iat;
    case win::directory_id::directory_entry_delay_import:
        return cont::DirectoryType::DelayImport;
    case win::directory_id::directory_entry_com_descriptor:
        return cont::DirectoryType::ComDescriptor;
    default:
        throw std::runtime_error("pe: unsupported directory type");
    }
}

constexpr cont::Section to_cont(const win::section_header_t section) {
    cont::Section result = {};
    result.name = section.name.to_string();
    result.virtual_size = static_cast<std::size_t>(section.virtual_size);
    result.virtual_address = static_cast<std::size_t>(section.virtual_address);
    result.size_raw_data = static_cast<std::size_t>(section.size_raw_data);
    result.ptr_raw_data = static_cast<std::size_t>(section.ptr_raw_data);
    result.characteristics.cnt_code = section.characteristics.cnt_code;
    result.characteristics.cnt_init_data = section.characteristics.cnt_init_data;
    result.characteristics.cnt_uninit_data = section.characteristics.cnt_uninit_data;
    result.characteristics.lnk_info = section.characteristics.lnk_info;
    result.characteristics.lnk_remove = section.characteristics.lnk_remove;
    result.characteristics.lnk_comdat = section.characteristics.lnk_comdat;
    result.characteristics.no_defer_spec_exc = section.characteristics.no_defer_spec_exc;
    result.characteristics.mem_far = section.characteristics.mem_far;
    result.characteristics.mem_purgeable = section.characteristics.mem_purgeable;
    result.characteristics.mem_locked = section.characteristics.mem_locked;
    result.characteristics.mem_preload = section.characteristics.mem_preload;
    result.characteristics.alignment = section.characteristics.alignment;
    result.characteristics.lnk_nreloc_ovfl = section.characteristics.lnk_nreloc_ovfl;
    result.characteristics.mem_discardable = section.characteristics.mem_discardable;
    result.characteristics.mem_not_cached = section.characteristics.mem_not_cached;
    result.characteristics.mem_not_paged = section.characteristics.mem_not_paged;
    result.characteristics.mem_shared = section.characteristics.mem_shared;
    result.characteristics.mem_execute = section.characteristics.mem_execute;
    result.characteristics.mem_read = section.characteristics.mem_read;
    result.characteristics.mem_write = section.characteristics.mem_write;
    return result;
}

template <typename Phdr>
    requires(traits::is_any_of_v<Phdr, Elf64_Phdr, Elf32_Phdr>)
[[nodiscard]] cont::Section to_cont(const Phdr& phdr) {
    cont::Section s{};
    s.name = std::nullopt;
    s.virtual_size = static_cast<std::size_t>(phdr.p_memsz);
    s.virtual_address = static_cast<std::size_t>(phdr.p_vaddr);
    s.size_raw_data = static_cast<std::size_t>(phdr.p_filesz);
    s.ptr_raw_data = static_cast<std::size_t>(phdr.p_offset);
    s.characteristics.mem_execute = !!(phdr.p_flags & PF_X);
    s.characteristics.mem_write = !!(phdr.p_flags & PF_W);
    s.characteristics.mem_read = !!(phdr.p_flags & PF_R);
    return s;
}

template <typename Shdr>
    requires(traits::is_any_of_v<Shdr, Elf64_Shdr, Elf32_Shdr>)
[[nodiscard]] cont::Section to_cont(const Shdr& shdr, const std::string_view name) {
    cont::Section s{};
    s.name = std::string{name};
    s.virtual_size = static_cast<std::size_t>(shdr.sh_size);
    s.virtual_address = static_cast<std::size_t>(shdr.sh_addr);
    s.size_raw_data = shdr.sh_type == SHT_NOBITS ? 0U : static_cast<std::size_t>(shdr.sh_size);
    s.ptr_raw_data = static_cast<std::size_t>(shdr.sh_offset);
    s.characteristics.mem_execute = !!(shdr.sh_flags & SHF_EXECINSTR);
    s.characteristics.mem_write = !!(shdr.sh_flags & SHF_WRITE);
    s.characteristics.mem_read = !!(shdr.sh_flags & SHF_ALLOC);
    return s;
}

[[nodiscard]] constexpr cont::RelocationType to_cont(const std::uint64_t elf_type, const cont::ImageMode mode) {
    switch (mode) {
    case cont::ImageMode::X64: {
        switch (elf_type) {
        case R_X86_64_64:
        case R_X86_64_RELATIVE:
            return cont::RelocationType::Dir64;
        case R_X86_64_32:
        case R_X86_64_32S:
            return cont::RelocationType::HighLow;
        case R_X86_64_GLOB_DAT:
            return cont::RelocationType::GlobDat64;
        default:
            throw std::out_of_range("cont::to_cont: Unsupported relocation type for x64 mode");
        }
    }
    case cont::ImageMode::X86: {
        switch (elf_type) {
        case R_386_32:
        case R_386_RELATIVE:
            return cont::RelocationType::HighLow;
        case R_386_GLOB_DAT:
            return cont::RelocationType::GlobDat32;
        default:
            throw std::out_of_range("cont::to_cont: Unsupported relocation type for x86 mode");
        }
    }
    default:
        throw std::out_of_range("cont::to_cont: Unsupported image mode");
    }
}

constexpr win::reloc_type_id to_win(const cont::RelocationType type) {
    switch (type) {
    case cont::RelocationType::Absolute:
        return win::reloc_type_id::rel_based_absolute;
    case cont::RelocationType::High:
        return win::reloc_type_id::rel_based_high;
    case cont::RelocationType::Low:
        return win::reloc_type_id::rel_based_low;
    case cont::RelocationType::HighLow:
        return win::reloc_type_id::rel_based_high_low;
    case cont::RelocationType::HighAdj:
        return win::reloc_type_id::rel_based_high_adj;
    case cont::RelocationType::Ia64Imm64:
        return win::reloc_type_id::rel_based_ia64_imm64;
    case cont::RelocationType::Dir64:
        return win::reloc_type_id::rel_based_dir64;
    }
    throw std::runtime_error("pe: unsupported relocation type");
}
