#include "func_parser/pdb/detail/parser_v7.hpp"
#include <es3n1n/common/logger.hpp>
#include <es3n1n/common/memory/address.hpp>

// \note: @es3n1n: s/o to @namazso for the stream related functions
namespace func_parser::pdb::detail {
    namespace {
        std::vector<std::uint8_t> get_stream_directory(const SuperBlock* header, const memory::address& raw) {
            const auto size = header->NumDirectoryBytes;
            const auto block_size = static_cast<std::ptrdiff_t>(header->BlockSize);
            const auto block_count = (size + block_size - 1) / block_size;

            if (size == 0 || block_count == 0 || block_size == 0) {
                logger::error("Empty stream directory, msg[0] size[{}] block_count[{}] block_size[{}]", size, block_count, block_size);
                return {};
            }

            std::vector<std::uint8_t> stream_dir;
            stream_dir.reserve(block_count * block_size);

            const auto* block_id_array = raw.offset(block_size * header->BlockMapAddr).cast<uint32_t*>();

            for (uint32_t i = 0; i < block_count; ++i) {
                const auto block = raw.offset(block_size * block_id_array[i]);
                stream_dir.insert(stream_dir.end(), block.cast<uint8_t*>(), //
                                  block.offset(block_size).cast<uint8_t*>());
            }

            if (stream_dir.empty()) {
                logger::error("Empty stream directory, msg[1] size[{}] block_count[{}] block_size[{}]", size, block_count, block_size);
                return {};
            }

            stream_dir.resize(size);
            return stream_dir;
        }
    } // namespace

    void V7Parser::read_header() {
        header_ = pdb_data_.cast<SuperBlock*>();

        if (std::memcmp(header_, kMicrosoftPdb7Magic.data(), kMicrosoftPdb7Magic.size()) != 0) {
            throw std::runtime_error("pdb: Invalid pdb7 header");
        }

        read_streams();
        read_dbi();
    }

    void V7Parser::read_streams() {
        const auto stream_directory = get_stream_directory(header_, pdb_data_);
        if (stream_directory.empty()) {
            throw std::runtime_error("pdb: Got empty stream directory");
        }

        const auto block_size = header_->BlockSize;

        const std::ptrdiff_t streams_count = memory::address{stream_directory.data()}.get<std::int32_t>().value();

        const auto* streams = memory::address{stream_directory.data()}.offset(sizeof(std::uint32_t)).cast<std::uint32_t*>();
        const auto* ids = memory::address{streams}.offset(streams_count * static_cast<std::ptrdiff_t>(sizeof(std::uint32_t))).cast<std::uint32_t*>();

        streams_.clear();
        streams_.reserve(streams_count);

        for (std::size_t i = 0; i < static_cast<std::size_t>(streams_count); ++i) {
            const auto stream_size = streams[i];
            const auto stream_blocks = (stream_size + block_size - 1) / block_size;

            std::vector<std::uint8_t> stream = {};
            stream.reserve(static_cast<std::size_t>(stream_blocks) * block_size);

            if (stream_blocks == 0) {
                // Handling empty streams
                // \note: @es3n1n: not sure how we should handle streams with size -1,
                // maybe somehow different, but if it works - it works.
                //
                if (static_cast<int32_t>(stream_size) <= 0) {
                    streams_.emplace_back();
                    continue;
                }

                // Resizing stream then
                //
                stream.resize(stream_size);
                streams_.emplace_back(std::move(stream));
                continue;
            }

            for (std::size_t j = 0; j < stream_blocks; ++j) {
                const auto block_id = *ids++;
                const auto block_start = pdb_data_.offset(static_cast<std::ptrdiff_t>(block_size) * block_id);

                stream.insert(stream.end(), block_start.cast<std::uint8_t*>(), block_start.offset(block_size).cast<std::uint8_t*>());
            }

            stream.resize(stream_size);
            streams_.emplace_back(std::move(stream));
        }
    }

    void V7Parser::read_dbi() {
        if (streams_.size() <= DBI_HEADER) [[unlikely]] {
            logger::error("pdb: DBI header not found, huh? streams_size[{}]", streams_.size());
            return;
        }

        const auto dbi_header_raw = streams_.at(DBI_HEADER);
        const auto* dbi_header = memory::address{dbi_header_raw}.cast<DBIHeader*>();
        if (dbi_header_raw.empty() || //
            dbi_header == nullptr) [[unlikely]] {
            logger::error("pdb: got empty DBI header, huh?");
            return;
        }

        const auto parse_symbol_records = [this, dbi_header]() -> void {
            if (streams_.size() <= dbi_header->SymRecordStream) [[unlikely]] {
                logger::warn("pdb: DBI sym record stream not found");
                return;
            }

            auto& sym_stream_raw = streams_.at(dbi_header->SymRecordStream);
            auto* sym_stream = sym_stream_raw.data();
            const auto* sym_stream_end = sym_stream + sym_stream_raw.size();

            for (; sym_stream != sym_stream_end; sym_stream += reinterpret_cast<DBIRecordHeader*>(sym_stream)->Size + 2) {
                dbi_symbols_[reinterpret_cast<DBIRecordHeader*>(sym_stream)->Kind].emplace_back(sym_stream);
            }
        };

        const auto parse_module_infos = [this, dbi_header]() -> void {
            if (dbi_header->ModInfoSize <= 0) [[unlikely]] {
                logger::warn("pdb: got empty DBI mod info stream");
                return;
            }

            auto iter = memory::address{dbi_header}.offset(sizeof(DBIHeader));
            const auto end = iter.offset(dbi_header->ModInfoSize);

            for (; iter < end; iter = iter.align_up(kDBIAlignment)) {
                const auto* module_info = iter.cast<DBIModuleInfo*>();
                iter = iter.offset(sizeof(*module_info)); // skip header

                const auto* module_name = iter.cast<char*>();
                iter = iter.offset(static_cast<std::ptrdiff_t>(std::strlen(module_name)) + 1); // skip module name + '\0'

                const auto* object_name = iter.cast<char*>();
                iter = iter.offset(static_cast<std::ptrdiff_t>(std::strlen(object_name)) + 1); // skip object name + '\0'

                // \note: @es3n1n: we don't need it to parse modules that have no info
                if (module_info->ModuleSymStream <= 0 || module_info->SymByteSize <= 0 || //
                    streams_.size() <= module_info->ModuleSymStream) [[unlikely]] {
                    continue;
                }

                const auto& raw_module_symbol_stream = streams_.at(module_info->ModuleSymStream);

                // \note: @es3n1n: +sizeof(uint32_t) because we are skipping unknown signature,
                // we only need symbols :shrug:
                auto sym_stream = memory::address{raw_module_symbol_stream.data()}.offset(sizeof(uint32_t));
                const auto sym_stream_end = sym_stream.offset(module_info->SymByteSize);

                for (; sym_stream < sym_stream_end; sym_stream = sym_stream.align_up(kDBIAlignment)) {
                    const auto* record = sym_stream.as<DBIRecordHeader*>();

                    // \note: @es3n1n: break on stream end
                    // \note: @es3n1n: for some reason stream doesn't end on the end marker?
                    // if (record->Kind == detail::e_symbol_kind::S_END) {
                    //    break;
                    //}

                    dbi_symbols_[record->Kind].emplace_back(record);

                    // +sizeof(uint16_t) and not sizeof(header) cos they are accounting the Size field from header and not Kind field
                    sym_stream = sym_stream.offset(static_cast<std::ptrdiff_t>(sizeof(uint16_t)) + record->Size);
                }
            }
        };

        const auto parse_image_section_stream = [this, dbi_header]() -> void {
            if (dbi_header->OptionalDbgHeaderSize <= 0) [[unlikely]] {
                logger::warn("pdb: got empty optional DBG header");
                return;
            }

            // \fixme: @es3n1n: is there a better way? i dont think so.
            const auto* optional_debug_header = memory::address{dbi_header} //
                                                    .offset(sizeof(DBIHeader)) //
                                                    .offset(dbi_header->ModInfoSize) //
                                                    .offset(dbi_header->SectionContributionSize) //
                                                    .offset(dbi_header->SectionMapSize) //
                                                    .offset(dbi_header->SourceInfoSize) //
                                                    .offset(dbi_header->TypeServerSize) //
                                                    .offset(dbi_header->ECSubStreamSize) //
                                                    .as<DBIOptionalDebugHeader*>();

            if (optional_debug_header->SectionHeaderStreamIndex <= 0 || //
                streams_.size() <= optional_debug_header->SectionHeaderStreamIndex) {
                logger::warn("pdb: got invalid optional DBG header");
                return;
            }

            auto& raw_image_section_stream = streams_.at(optional_debug_header->SectionHeaderStreamIndex);
            auto* image_section_stream = reinterpret_cast<IMAGE_SECTION_HEADER*>(raw_image_section_stream.data());

            for (std::size_t i = 0; i < (raw_image_section_stream.size() / sizeof(IMAGE_SECTION_HEADER)); ++i) {
                auto* image_section = &image_section_stream[i];

                sections_.emplace_back(image_section->VirtualAddress);
            }
        };

        parse_symbol_records();
        parse_module_infos();
        parse_image_section_stream();

        logger::debug("pdb: Parsed {} types of DBI symbols", dbi_symbols_.size());
    }

    void V7Parser::iter_symbols(const std::function<bool(std::uint16_t, memory::address)>& callback) const {
        for (const auto& [kind, pointers] : dbi_symbols_) {
            for (const auto& pointer : pointers) {
                if (callback(kind, pointer)) {
                    continue;
                }

                return;
            }
        }
    }
} // namespace func_parser::pdb::detail