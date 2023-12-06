#pragma once
#include <array>
#include <cstdint>

// @credits: llvm <3
namespace func_parser::pdb::detail {
    // Microsoft C/C++ MSF 7.00 ...
    //
    constexpr std::array<std::uint8_t, 32> kMicrosoftPdb7Magic = {0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x43,
                                                                  0x2F, 0x43, 0x2B, 0x2B, 0x20, 0x4D, 0x53, 0x46, 0x20, 0x37, 0x2E,
                                                                  0x30, 0x30, 0x0D, 0x0A, 0x1A, 0x44, 0x53, 0x00, 0x00, 0x00};

    constexpr std::size_t kDBIAlignment = sizeof(uint32_t);

    enum e_stream_index : std::uint8_t {
        DBI_HEADER = 3,
    };

    enum e_symbol_kind : std::uint16_t {
        S_LPROC32 = 0x110FU, // local fn
        S_GPROC32 = 0x1110U, // global fn
        S_END = 0x6, // end of mod symbols
    };

#pragma pack(push, 1)
    struct SuperBlock {
        std::array<char, kMicrosoftPdb7Magic.size()> FileMagic;
        uint32_t BlockSize;
        uint32_t FreeBlockMapBlock;
        uint32_t NumBlocks;
        uint32_t NumDirectoryBytes;
        uint32_t Unknown;
        uint32_t BlockMapAddr;
    };

    struct DBIHeader {
        int32_t VersionSignature;
        uint32_t VersionHeader;
        uint32_t Age;
        uint16_t GlobalStreamIndex;
        uint16_t BuildNumber;
        uint16_t PublicStreamIndex;
        uint16_t PdbDllVersion;
        uint16_t SymRecordStream;
        uint16_t PdbDllRebuild;
        int32_t ModInfoSize;
        int32_t SectionContributionSize;
        int32_t SectionMapSize;
        int32_t SourceInfoSize;
        int32_t TypeServerSize;
        uint32_t MFCTypeServerIndex;
        int32_t OptionalDbgHeaderSize;
        int32_t ECSubStreamSize;
        uint16_t Flags;
        uint16_t Machine;
        uint32_t Padding;
    };

    struct DBIModuleInfo {
        uint32_t Unused1;
        struct SectionContribEntry {
            uint16_t Section;
            // NOLINTNEXTLINE
            char Padding1[2];
            int32_t Offset;
            int32_t Size;
            uint32_t Characteristics;
            uint16_t ModuleIndex;
            // NOLINTNEXTLINE
            char Padding2[2];
            uint32_t DataCrc;
            uint32_t RelocCrc;
        } SectionContr;
        uint16_t Flags;
        uint16_t ModuleSymStream;
        uint32_t SymByteSize;
        uint32_t C11ByteSize;
        uint32_t C13ByteSize;
        uint16_t SourceFileCount;
        // NOLINTNEXTLINE
        char Padding[2];
        uint32_t Unused2;
        uint32_t SourceFileNameIndex;
        uint32_t PdbFilePathNameIndex;

        // char ModuleName[];
        // char ObjFileName[];
    };

    struct DBIOptionalDebugHeader { // tyty rawpdb
        uint16_t FpoDataStreamIndex; // IMAGE_DEBUG_TYPE_FPO
        uint16_t ExceptionDataStreamIndex; // IMAGE_DEBUG_TYPE_EXCEPTION
        uint16_t FixupDataStreamIndex; // IMAGE_DEBUG_TYPE_FIXUP
        uint16_t OmapToSrcDataStreamIndex; // IMAGE_DEBUG_TYPE_OMAP_TO_SRC
        uint16_t OmapFromSrcDataStreamIndex; // IMAGE_DEBUG_TYPE_OMAP_FROM_SRC
        uint16_t SectionHeaderStreamIndex; // a dump of all section headers (IMAGE_SECTION_HEADER) from the original executable
        uint16_t TokenDataStreamIndex;
        uint16_t XDataStreamIndex;
        uint16_t PDataStreamIndex;
        uint16_t NewFpoDataStreamIndex;
        uint16_t OriginalSectionHeaderDataStreamIndex;
    };

    struct IMAGE_SECTION_HEADER {
        // NOLINTNEXTLINE
        uint8_t Name[8];
        union {
            uint32_t PhysicalAddress;
            uint32_t VirtualSize;
        } Misc;
        uint32_t VirtualAddress;
        uint32_t SizeOfRawData;
        uint32_t PointerToRawData;
        uint32_t PointerToRelocations;
        uint32_t PointerToLineNumbers;
        uint16_t NumberOfRelocations;
        uint16_t NumberOfLineNumbers;
        uint32_t Characteristics;
    };

    struct DBIRecordHeader {
        uint16_t Size;
        uint16_t Kind;
    };

    struct DBIRecordPUB32 {
        DBIRecordHeader Header;
        uint16_t PubSymFlags;
        uint32_t Offset;
        uint16_t Segment;
    };

    struct DBIRecordProc32 {
        DBIRecordHeader Header;
        uint32_t Parent;
        uint32_t End;
        uint32_t Next;
        uint32_t Size;
        uint32_t DebugStart;
        uint32_t DebugEnd;
        uint32_t TypeIndex;
        uint32_t Offset;
        uint16_t Segment;
        uint8_t Flags;
        // NOLINTNEXTLINE
        char Name[1];
    };

    struct DBIRecordLProc32 : DBIRecordProc32 { };
    struct DBIRecordGProc32 : DBIRecordProc32 { };
#pragma pack(pop)
} // namespace func_parser::pdb::detail
