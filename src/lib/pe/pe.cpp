#include "pe/pe.hpp"

#include "pe/arch/arch.hpp"
#include "pe/common/common.hpp"
#include "pe/debug/debug.hpp"
#include "pe/rebuilder/rebuilder.hpp"

#include "util/format.hpp"
#include <es3n1n/common/logger.hpp>
#include <magic_enum_all.hpp>

namespace pe {
    template <any_raw_image_t Img>
    [[nodiscard]] bool Image<Img>::is_x64() const {
        return arch::is_x64(raw_image);
    }

    template <any_raw_image_t Img>
    [[nodiscard]] bool Image<Img>::is_valid() const {
        return common::is_valid(raw_image);
    }

    template <any_raw_image_t Img>
    [[nodiscard]] std::vector<section_t> Image<Img>::find_sections_if(const std::function<bool(const section_t&)>& pred) const {
        std::vector<section_t> result = {};

        std::for_each(sections.begin(), sections.end(), [&result, pred](const section_t& section) -> void {
            if (!pred(section)) {
                return;
            }

            result.emplace_back(section);
        });

        return result;
    }

    template <any_raw_image_t Img>
    [[nodiscard]] win::cv_pdb70_t* Image<Img>::find_codeview70() const {
        return debug::find_codeview70(raw_image);
    }

    template <any_raw_image_t Img>
    [[nodiscard]] zasm::MachineMode Image<Img>::guess_machine_mode() const {
        return arch::guess_machine_mode(raw_image);
    }

    template <any_raw_image_t Img>
    [[nodiscard]] section_t& Image<Img>::find_last_section() const {
        auto result = std::ranges::max_element(sections, [](const section_t& lhs, const section_t& rhs) -> bool { //
            return lhs.virtual_address < rhs.virtual_address;
        });

        if (result == std::end(sections)) {
            throw std::runtime_error("pe: Unable to find last section");
        }

        return *result;
    }

    template <any_raw_image_t Img>
    section_t& Image<Img>::new_section(const sections::e_section_t section, const std::size_t size) {
        return new_section(format::sec(section), size, characteristics(section));
    }

    template <any_raw_image_t Img>
    section_t& Image<Img>::new_section(const std::string_view name, const std::size_t size, //
                                       const win::section_characteristics_t characteristics) {
        // Creating a new section.
        // \note: @es3n1n: we must create a new section before the std::max_element call in
        // order to not corrupt the iterator
        auto& new_sec = sections.emplace_back();

        // Reserving section size
        //
        new_sec.raw_data.resize(size, 0);
        assert(size > 0);

        // Obtaining last section
        //
        auto last_section = find_last_section();

        // Obtaining section/file alignment values
        //
        const auto section_alignment = raw_image->get_nt_headers()->optional_header.section_alignment;
        const auto file_alignment = raw_image->get_nt_headers()->optional_header.file_alignment;

        // Assembling the new section
        //
        std::copy_n(name.begin(), std::min(new_sec.name.size() - 1, name.size()), new_sec.name.begin());

        new_sec.virtual_size = new_sec.size_raw_data = memory::address{size} //
                                                           .align_up(section_alignment)
                                                           .template as<uint32_t>();

        new_sec.virtual_address = memory::address{last_section.virtual_address + last_section.virtual_size} //
                                      .align_up(section_alignment)
                                      .template as<uint32_t>();

        new_sec.ptr_raw_data = memory::address{last_section.ptr_raw_data + last_section.size_raw_data} //
                                   .align_up(file_alignment)
                                   .template as<uint32_t>();

        new_sec.characteristics = characteristics;

        return new_sec;
    }

    template <any_raw_image_t Img>
    void Image<Img>::realign_sections() const {
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

    template <any_raw_image_t Img>
    [[nodiscard]] section_t* Image<Img>::rva_to_section(std::uint32_t rva) const {
        auto iter = std::ranges::find_if(sections, [rva](const section_t& sec) -> bool { //
            return rva >= sec.virtual_address && rva <= (sec.virtual_address + sec.virtual_size);
        });

        if (iter == sections.end()) {
            return nullptr;
        }

        return &*iter;
    }

    template <any_raw_image_t Img>
    void Image<Img>::update_sections() {
        // Obtaining stuff that would be needed
        //
        const auto* nt_hdr = raw_image->get_nt_headers();

        // Reserving the num of sections
        //
        sections.clear();
        sections.reserve(nt_hdr->file_header.num_sections);

        for (std::size_t i = 0; i < nt_hdr->file_header.num_sections; ++i) {
            // Getting a section and validating it
            //
            const auto* section = nt_hdr->get_section(i);
            if (section == nullptr) {
                continue;
            }

            // Inserting a section to the result array
            //
            auto& new_elem = sections.emplace_back(*section);
            new_elem.raw_data.resize(new_elem.size_raw_data, 0);

            // NOLINTNEXTLINE
            std::memcpy(new_elem.raw_data.data(), reinterpret_cast<std::uint8_t*>(reinterpret_cast<uintptr_t>(raw_image) + new_elem.ptr_raw_data), //
                        new_elem.size_raw_data);
        }

        // Signalising that we successfully parsed sections
        //
        logger::debug("pe: parsed {} sections", sections.size());

        // Sort by virtual address
        //
        const struct {
            bool operator()(const section_t& lhs, const section_t& rhs) const {
                return lhs.virtual_address < rhs.virtual_address;
            }
        } comp;
        std::sort(sections.begin(), sections.end(), comp);

        // Marking directories
        //
        for (auto dir_id : magic_enum::enum_values<win::directory_id>()) {
            // Trying to find the header of the directory, skipping if not present
            //
            auto dir_hdr = raw_image->get_directory(dir_id);
            if (!dir_hdr || !dir_hdr->present()) {
                continue;
            }

            // Looking up the section by rva and changing flag
            //
            auto* sec = rva_to_section(dir_hdr->rva);
            sec->set_contained_dir(dir_id, dir_hdr->rva - sec->virtual_address, dir_hdr->size);
        }
    }

    // \todo: @es3n1n: check for IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE flag
    template <any_raw_image_t Img>
    void Image<Img>::update_relocations() {
        // Obtaining a pointer to reloc directory header
        //
        const win::data_directory_t* reloc_hdr = raw_image->get_directory(win::directory_id::directory_entry_basereloc);
        if ((reloc_hdr == nullptr) || !reloc_hdr->present()) [[unlikely]] {
            logger::warn("pe: relocation directory header does not present?");
            return;
        }

        // Obtaining a pointer to reloc directory
        //
        const win::reloc_directory_t* base_reloc = rva_to_ptr<win::reloc_directory_t>(reloc_hdr->rva);

        // Guessing the relocated ptr size
        // \todo: @es3n1n: is this even needed?
        const auto reloc_size = get_ptr_size<std::uint8_t>();

        // Iterating over reloc blocks
        //
        for (const auto* reloc_block = &base_reloc->first_block; //
             (reloc_block != nullptr) && (reloc_block->size_block != 0U) && (reloc_block->base_rva != 0U); //
             reloc_block = reloc_block->next()) {
            // Iterating over reloc entries
            //
            for (const auto& [offset, type] : *reloc_block) {
                // Skip ignored relocations
                //
                if (type == win::reloc_type_id::rel_based_absolute) {
                    continue;
                }

                // Inserting parsed reloc data
                //
                auto rva = static_cast<memory::address>(reloc_block->base_rva) + memory::address(offset);

                // Just to be sure
                //
                //if (relocations.contains(rva)) [[unlikely]] {
                //    throw std::runtime_error(std::format("pe: duplicated {:#x} rva entry", rva));
                //}

                // Inserting relocation info
                //
                relocations[rva] = relocation_t{
                    .rva = rva, //
                    .size = reloc_size, //
                    .type = type //
                };
            }
        }

        logger::debug("pe: parsed total number of {} relocations", relocations.size());
    }

    template <any_raw_image_t Img>
    [[nodiscard]] std::vector<std::uint8_t> Image<Img>::rebuild_pe_image() {
        auto ctx = rebuilder_ctx_t<Image>{.image = this};
        return rebuild_pe(ctx);
    }

    template class Image<win::image_x64_t>;
    template class Image<win::image_x86_t>;
} // namespace pe