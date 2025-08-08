#pragma once
#include <cont/base.hpp>
#include <linux/elf.h>
#include <numeric>

namespace cont::elf {
    struct Dynamic {
        std::uint64_t tag = 0;
        std::uint64_t value = 0;
    };

    class Image final : public ImageBase {
    public:
        explicit Image(const memory::address raw_image): ImageBase(ContImageType::ELF, raw_image) {
            initialize();
        }
        [[nodiscard]] ImageMode mode() const override;
        [[nodiscard]] bool verify_integrity() const override;

        [[nodiscard]] std::size_t get_image_base() const override;
        [[nodiscard]] std::size_t get_section_alignment() const override;
        [[nodiscard]] std::size_t get_file_alignment() const override;

        [[nodiscard]] std::vector<std::uint8_t> rebuild_image() override;

        void update_sections() override;
        void update_relocations() override;

        [[nodiscard]] std::size_t non_symbolic_segments_count() const {
            return std::accumulate(sections.begin(), sections.end(), static_cast<std::size_t>(0),
                                   [](const auto acc, const Section& sec) -> std::size_t { return acc + (sec.symbolic ? 0 : 1); });
        }

        [[nodiscard]] Section* find_segment_with_type(const std::size_t type) {
            const auto iter = std::ranges::find_if(sections, [type](const Section& sec) -> bool { //
                return sec.elf_type.value_or(PT_LOAD) == type;
            });
            if (iter == sections.end()) {
                return nullptr;
            }

            return &*iter;
        }

        [[nodiscard]] Section* rva_to_section(std::uint32_t rva) override {
            const auto iter = std::ranges::find_if(sections, [rva](const Section& sec) -> bool { //
                return sec.elf_type.value_or(PT_LOAD) == PT_LOAD && //
                       rva >= sec.virtual_address && rva <= (sec.virtual_address + sec.virtual_size);
            });

            if (iter == sections.end()) {
                return nullptr;
            }

            return &*iter;
        }

        [[nodiscard]] Dynamic* find_dynamic(const std::size_t tag) {
            const auto it = std::ranges::find_if(dynamics, [tag](const Dynamic& dyn) { return dyn.tag == tag; });
            if (it == dynamics.end()) {
                return nullptr;
            }

            return &*it;
        }

        void delete_dynamic(const std::size_t tag) {
            std::erase_if(dynamics, [tag](const Dynamic& dyn) { return dyn.tag == tag; });
        }

        std::vector<Dynamic> dynamics;

    private:
        std::unordered_map<memory::address, Relocation> jmprel_relocations;

        [[nodiscard]] Elf64_Ehdr* x64() const {
            return raw_image_.as<Elf64_Ehdr*>();
        }

        [[nodiscard]] Elf32_Ehdr* x86() const {
            return raw_image_.as<Elf32_Ehdr*>();
        }

        template <typename ElfEhdr, typename ElfPhdr>
        [[nodiscard]] static std::span<const ElfPhdr> phdr_span(const ElfEhdr* ehdr) {
            const auto* first = reinterpret_cast<const ElfPhdr*>(reinterpret_cast<const std::byte*>(ehdr) + ehdr->e_phoff);
            return {first, static_cast<std::size_t>(ehdr->e_phnum)};
        }

        template <typename ElfEhdr, typename ElfShdr>
        [[nodiscard]] static std::span<const ElfShdr> shdr_span(const ElfEhdr* ehdr) {
            const auto* first = reinterpret_cast<const ElfShdr*>(reinterpret_cast<const std::byte*>(ehdr) + ehdr->e_shoff);
            return {first, static_cast<std::size_t>(ehdr->e_shnum)};
        }
    };

    template <typename Ty> concept AnyRawImage = traits::is_any_of_v<Ty, Elf64_Ehdr, Elf32_Ehdr>;
} // namespace cont::elf
