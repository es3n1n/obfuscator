#pragma once
#include "util/structs.hpp"
#include "util/types.hpp"
#include <es3n1n/common/memory/address.hpp>
#include <linuxpe>
#include <optional>

#define TOGGLE_DIR_IMPORTER(id, name) \
    case id:                          \
        (name) = make();              \
        return

#define TOGGLE_DIR_EXPORTER(id, name) \
    if ((name).has_value()) {         \
        set(id, (name).value());      \
    }

#define TOGGLE_DIRECTORIES(fn)                                                   \
    fn(win::directory_id::directory_entry_export, contains_dir.exp);             \
    fn(win::directory_id::directory_entry_import, contains_dir.imp);             \
    fn(win::directory_id::directory_entry_resource, contains_dir.rsc);           \
    fn(win::directory_id::directory_entry_exception, contains_dir.exc);          \
    fn(win::directory_id::directory_entry_security, contains_dir.sec);           \
    fn(win::directory_id::directory_entry_basereloc, contains_dir.reloc);        \
    fn(win::directory_id::directory_entry_debug, contains_dir.dbg);              \
    fn(win::directory_id::directory_entry_architecture, contains_dir.arch);      \
    fn(win::directory_id::directory_entry_globalptr, contains_dir.gp);           \
    fn(win::directory_id::directory_entry_tls, contains_dir.tls);                \
    fn(win::directory_id::directory_entry_load_config, contains_dir.cfg);        \
    fn(win::directory_id::directory_entry_bound_import, contains_dir.imp_bound); \
    fn(win::directory_id::directory_entry_iat, contains_dir.iat);                \
    fn(win::directory_id::directory_entry_delay_import, contains_dir.imp_delay); \
    fn(win::directory_id::directory_entry_com_descriptor, contains_dir.com)

namespace pe {
    struct relocation_t {
        memory::address rva;
        std::uint8_t size = 0; // in bytes
        win::reloc_type_id type = win::reloc_type_id::rel_based_absolute;
    };

    struct dir_properties_t {
        std::uint32_t offset; // from the section start
        std::size_t size; // in bytes
    };

    struct section_t {
        DEFAULT_CT_CTOR_DTOR(section_t);
        DEFAULT_COPY(section_t);

        explicit section_t(const win::section_header_t& header)
            : virtual_size(header.virtual_size), virtual_address(header.virtual_address), size_raw_data(header.size_raw_data),
              ptr_raw_data(header.ptr_raw_data), characteristics(header.characteristics) {
            std::memcpy(name.data(), reinterpret_cast<const char*>(header.name.short_name), name.size() * sizeof(decltype(name)::value_type));
        }

        std::array<char, LEN_SHORT_STR> name = {};
        std::uint32_t virtual_size = 0U;
        std::uint32_t virtual_address = 0U;
        std::uint32_t size_raw_data = 0U;
        std::uint32_t ptr_raw_data = 0U;
        win::section_characteristics_t characteristics = {};

        std::vector<std::uint8_t> raw_data;

        /// This struct contains directory offsets within the section, i.e
        /// If a section contains import descriptors the value of iat would be set
        /// to the offset where iat starts within this section, otherwise would be
        /// set to nullopt
        // \fixme: @es3n1n: THIS IS SO WRONG, BUT I DONT WANT TO STORE THEM IN CONTAINERS IM SO SORRY
        struct {
            std::optional<dir_properties_t> exp = std::nullopt; // export dir
            std::optional<dir_properties_t> imp = std::nullopt; // import dir
            std::optional<dir_properties_t> rsc = std::nullopt; // resource dir
            std::optional<dir_properties_t> exc = std::nullopt; // exception dir
            std::optional<dir_properties_t> sec = std::nullopt; // security dir
            std::optional<dir_properties_t> reloc = std::nullopt; // reloc dir
            std::optional<dir_properties_t> dbg = std::nullopt; // debug dir
            std::optional<dir_properties_t> arch = std::nullopt; // architecture dir
            std::optional<dir_properties_t> gp = std::nullopt; // GP dir
            std::optional<dir_properties_t> tls = std::nullopt; // tls dir
            std::optional<dir_properties_t> cfg = std::nullopt; // config dir
            std::optional<dir_properties_t> imp_bound = std::nullopt; // bound import dir
            std::optional<dir_properties_t> iat = std::nullopt; // import address table dir
            std::optional<dir_properties_t> imp_delay = std::nullopt; // delay import dir
            std::optional<dir_properties_t> com = std::nullopt; // COM descriptor dir
        } contains_dir;

        void set_contained_dir(const win::directory_id dir_id, const std::uint32_t offset, const std::optional<std::size_t> size = std::nullopt) {
            auto make = [&]() -> std::optional<dir_properties_t> {
                return dir_properties_t{.offset = offset, .size = size.has_value() ? size.value() : (virtual_size - offset)};
            };

            switch (dir_id) {
            case win::directory_id::directory_reserved0:
                break;
                TOGGLE_DIRECTORIES(TOGGLE_DIR_IMPORTER);
            }

            std::unreachable();
        }

        template <typename Ty>
            requires(traits::is_any_of_v<Ty, win::optional_header_x64_t, win::optional_header_x86_t>)
        void export_contained_dir(Ty* optional_header) {
            auto set = [this, &optional_header](win::directory_id dir_id, const dir_properties_t props) -> void {
                auto& dir = optional_header->data_directories.entries[dir_id];

                if (dir.rva != 0UL || dir.size != 0UL) {
                    throw std::runtime_error(std::format("pe: export_contained_dir duplicated dir_id {} in section {}", //
                                                         static_cast<int>(dir_id), name.data()));
                }

                dir.rva = virtual_address + props.offset;
                dir.size = static_cast<std::uint32_t>(props.size);
            };

            TOGGLE_DIRECTORIES(TOGGLE_DIR_EXPORTER);
        }

        /// Some explicit conversions to other linux-pe stuff
        explicit operator win::section_header_t() const {
            win::section_header_t result{};

            std::ranges::copy(name, reinterpret_cast<char*>(result.name.short_name));
            result.virtual_size = virtual_size;
            result.virtual_address = virtual_address;
            result.size_raw_data = size_raw_data;
            result.ptr_raw_data = ptr_raw_data;
            result.characteristics = characteristics;
            return result;
        }
    };
} // namespace pe

#undef TOGGLE_DIRECTORIES
#undef TOGGLE_DIR_EXPORTER
#undef TOGGLE_DIR_IMPORTER
