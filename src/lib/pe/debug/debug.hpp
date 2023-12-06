#pragma once
#include "pe/pe.hpp"

namespace pe::debug {
    template <any_raw_image_t Img>
    [[nodiscard]] win::cv_pdb70_t* find_codeview70(const Img* image) {
        // Looking for debug directory in our PE
        //
        auto* const debug_dir_hdr = image->get_directory(win::directory_id::directory_entry_debug);
        if (!debug_dir_hdr || !debug_dir_hdr->present()) {
            return nullptr;
        }

        // Should never happen
        //
        auto* debug_dir = image->template rva_to_ptr<win::debug_directory_t>(debug_dir_hdr->rva);
        if (!debug_dir) [[unlikely]] {
            return nullptr;
        }

        for (auto* entry = &debug_dir->entries[0]; entry->size_raw_data; ++entry) {
            if (entry->type != win::debug_directory_type_id::codeview) {
                continue;
            }

            // Yay, we found our code view from the debug section
            // NOLINTNEXTLINE
            return reinterpret_cast<win::cv_pdb70_t*>(reinterpret_cast<std::uintptr_t>(image) + entry->ptr_raw_data);
        }

        // No code view
        //
        return nullptr;
    }
} // namespace pe::debug
