#include "cont/elf/rebuilder/rebuilder.hpp"

namespace cont::elf::detail {
    namespace {
        template <AnyRawImage Img, typename Ph>
            requires(traits::is_any_of_v<Ph, Elf64_Phdr, Elf32_Phdr>)
        void init_header_(Image* image, std::vector<std::uint8_t>& data) {
            const auto* ehdr = image->raw_image().as<const Img*>();
            auto phdrs = image->phdr_span<Img, Ph>(ehdr);

            const auto& last = image->find_last_section();
            const std::size_t raw_image_size = last.ptr_raw_data + last.size_raw_data;
            const std::size_t virtual_image_size = last.virtual_address + last.virtual_size;

            auto* mutable_phdrs = image->raw_image().offset(ehdr->e_phoff).as<Ph*>();
            for (std::size_t i = 0; i < phdrs.size(); ++i) {
                if (Ph& ph = mutable_phdrs[i]; ph.p_type == PT_LOAD && ph.p_offset == 0) {
                    ph.p_filesz = static_cast<decltype(ph.p_filesz)>(raw_image_size);
                    ph.p_memsz = static_cast<decltype(ph.p_memsz)>(virtual_image_size);
                    break;
                }
            }

            const std::size_t header_size = ehdr->e_phoff + ehdr->e_phentsize * ehdr->e_phnum;
            data.resize(raw_image_size);
            std::memcpy(data.data(), image->raw_image().ptr<const void*>(), header_size);
        }
    } // namespace

    void init_header(Image* image, std::vector<std::uint8_t>& data) {
        switch (image->mode()) {
        case ImageMode::X64: {
            init_header_<Elf64_Ehdr, Elf64_Phdr>(image, data);
            break;
        }
        case ImageMode::X86: {
            init_header_<Elf32_Ehdr, Elf32_Phdr>(image, data);
            break;
        }
        default:
            throw std::out_of_range("cont::elf::detail::init_header: Unsupported image mode");
        }
    }
} // namespace cont::elf::detail
