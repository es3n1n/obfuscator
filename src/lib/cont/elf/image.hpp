#pragma once
#include <cont/base.hpp>
#include <linux/elf.h>

namespace cont::elf {
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
