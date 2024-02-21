#pragma once
#include <stdint.h>
#include <array>
#include <windows.h>
#include <widemath.h>

#pragma pack(push)
#pragma pack(1)

enum class DumpType_t : uint32_t {
    // Old dump types from dbgeng.dll
    FullDump = 0x1,
    KernelDump = 0x2,
    BMPDump = 0x5,
    CrashDump = 0x6,

    // New stuff
    MiniDump = 0x4,                // Produced by `.dump /m`
    KernelMemoryDump = 0x8,        // Produced by `.dump /k`
    KernelAndUserMemoryDump = 0x9, // Produced by `.dump /ka`
    CompleteMemoryDump = 0xa,      // Produced by `.dump /f`
};

struct PHYSMEM_RUN {
    uint64_t BasePage;
    uint64_t PageCount;
};

struct PHYSMEM_DESC {
    uint32_t NumberOfRuns;
    uint32_t Padding0;
    uint64_t NumberOfPages;
    PHYSMEM_RUN Run[1];

    constexpr bool LooksGood() const {
        if (NumberOfRuns == 0x45474150 || NumberOfPages == 0x4547415045474150ULL) {
            return false;
        }

        return true;
    }
};

union DUMP_FILE_ATTRIBUTES {
    struct DUMP_FILE_ATTRIBUTES_0 {
        uint32_t _bitfield;
    } Anonymous;
    uint32_t Attributes;
};

struct BMP_HEADER64 {
    static constexpr uint32_t ExpectedSignature = 0x50'4D'44'53;  // 'PMDS'
    static constexpr uint32_t ExpectedSignature2 = 0x50'4D'44'46; // 'PMDF'
    static constexpr uint32_t ExpectedValidDump = 0x50'4D'55'44;  // 'PMUD'

    //
    // Should be FDMP.
    //

    uint32_t Signature;

    //
    // Should be DUMP.
    //

    uint32_t ValidDump;

    //
    // According to rekall there's a gap there:
    // 'ValidDump': [0x4, ['String', dict(
    //    length=4,
    //    term=None,
    //    )]],
    // # The offset of the first page in the file.
    // 'FirstPage': [0x20, ['unsigned long long']],
    //

    std::array<uint8_t, 0x20 - (0x4 + sizeof(ValidDump))> Padding0;

    //
    // The offset of the first page in the file.
    //

    uint64_t FirstPage;

    //
    // Total number of pages present in the bitmap.
    //
    uint64_t TotalPresentPages;

    //
    // Total number of pages in image.This dictates the total size of the
    // bitmap.This is not the same as the TotalPresentPages which is only
    // the sum of the bits set to 1.
    //

    uint64_t Pages;

    std::array<uint8_t, 1> Bitmap;

    bool LooksGood() const {

        //
        // Integrity check the headers.
        //

        if (Signature != ExpectedSignature && Signature != ExpectedSignature2) {
            return false;
        }

        if (ValidDump != ExpectedValidDump) {
            return false;
        }

        return true;
    }
};

struct RDMP_HEADER64 {
    static constexpr uint32_t ExpectedMarker = 0x40;
    static constexpr uint32_t ExpectedSignature = 0x50'4D'44'52; // 'PMDR'
    static constexpr uint32_t ExpectedValidDump = 0x50'4D'55'44; // 'PMUD'

    uint32_t Marker;
    uint32_t Signature;
    uint32_t ValidDump;
    uint32_t __Unused;
    uint64_t MetadataSize;
    uint64_t FirstPageOffset;

    bool LooksGood() const {
        if (Marker != ExpectedMarker) {
            return false;
        }

        if (Signature != RDMP_HEADER64::ExpectedSignature) {
            return false;
        }

        if (ValidDump != RDMP_HEADER64::ExpectedValidDump) {
            return false;
        }

        if (MetadataSize - 0x20 !=
            FirstPageOffset -
            0x20'40) { // sizeof(HEADER64) + sizeof(RDMP_HEADERS64)
            return false;
        }

        return true;
    }
};

struct KERNEL_RDMP_HEADER64 {
    RDMP_HEADER64 Hdr;
    uint64_t __Unknown1;
    uint64_t __Unknown2;
    std::array<uint8_t, 1> Bitmap;
};

struct FULL_RDMP_HEADER64 {
    RDMP_HEADER64 Hdr;
    uint32_t NumberOfRanges;
    uint16_t __Unknown1;
    uint16_t __Unknown2;
    uint64_t TotalNumberOfPages;
    std::array<uint8_t, 1> Bitmap;
};

struct CONTEXT_STRUCT {

    //
    // Note that the below definition has been stolen directly from the windows
    // headers. Why you might ask? Well the structure comes with DECLSPEC_ALIGN
    // that was preventing me from layoung the Context structure at the offset I
    // wanted. Maybe there's a cleaner way to do this, if so let me know :)
    //

    //
    // Register parameter home addresses.
    //
    // N.B. These fields are for convience - they could be used to extend the
    //      context record in the future.
    //

    uint64_t P1Home;
    uint64_t P2Home;
    uint64_t P3Home;
    uint64_t P4Home;
    uint64_t P5Home;
    uint64_t P6Home;

    //
    // Control flags.
    //

    uint32_t ContextFlags;
    uint32_t MxCsr;

    //
    // Segment Registers and processor flags.
    //

    uint16_t SegCs;
    uint16_t SegDs;
    uint16_t SegEs;
    uint16_t SegFs;
    uint16_t SegGs;
    uint16_t SegSs;
    uint32_t EFlags;

    //
    // Debug registers
    //

    uint64_t Dr0;
    uint64_t Dr1;
    uint64_t Dr2;
    uint64_t Dr3;
    uint64_t Dr6;
    uint64_t Dr7;

    //
    // Integer registers.
    //

    uint64_t Rax;
    uint64_t Rcx;
    uint64_t Rdx;
    uint64_t Rbx;
    uint64_t Rsp;
    uint64_t Rbp;
    uint64_t Rsi;
    uint64_t Rdi;
    uint64_t R8;
    uint64_t R9;
    uint64_t R10;
    uint64_t R11;
    uint64_t R12;
    uint64_t R13;
    uint64_t R14;
    uint64_t R15;

    //
    // Program counter.
    //

    uint64_t Rip;

    //
    // Floating point state.
    //

    uint16_t ControlWord;
    uint16_t StatusWord;
    uint8_t TagWord;
    uint8_t Reserved1;
    uint16_t ErrorOpcode;
    uint32_t ErrorOffset;
    uint16_t ErrorSelector;
    uint16_t Reserved2;
    uint32_t DataOffset;
    uint16_t DataSelector;
    uint16_t Reserved3;
    uint32_t MxCsr2;
    uint32_t MxCsr_Mask;
    std::array<uint128_t, 8> FloatRegisters;
    uint128_t Xmm0;
    uint128_t Xmm1;
    uint128_t Xmm2;
    uint128_t Xmm3;
    uint128_t Xmm4;
    uint128_t Xmm5;
    uint128_t Xmm6;
    uint128_t Xmm7;
    uint128_t Xmm8;
    uint128_t Xmm9;
    uint128_t Xmm10;
    uint128_t Xmm11;
    uint128_t Xmm12;
    uint128_t Xmm13;
    uint128_t Xmm14;
    uint128_t Xmm15;

    //
    // Vector registers.
    //

    std::array<uint128_t, 26> VectorRegister;
    uint64_t VectorControl;

    //
    // Special debug control registers.
    //

    uint64_t DebugControl;
    uint64_t LastBranchToRip;
    uint64_t LastBranchFromRip;
    uint64_t LastExceptionToRip;
    uint64_t LastExceptionFromRip;

    bool LooksGood() const {

        //
        // Integrity check the CONTEXT record.
        //

        if (MxCsr != MxCsr2) {
            printf("CONTEXT::MxCsr doesn't match MxCsr2.\n");
            return false;
        }

        return true;
    }
};

// https://microsoft.github.io/windows-docs-rs/doc/windows/Win32/System/Diagnostics/Debug/struct.DUMP_HEADER64.html#structfield.DumpType
struct HEADER64 {
    static constexpr uint32_t ExpectedSignature = 0x45474150; // 'EGAP'
    static constexpr uint32_t ExpectedValidDump = 0x34365544; // '46UD'

    /* 0x0000 */ uint32_t Signature;
    /* 0x0004 */ uint32_t ValidDump;
    /* 0x0008 */ uint32_t MajorVersion;
    /* 0x000c */ uint32_t MinorVersion;
    /* 0x0010 */ uint64_t DirectoryTableBase;
    /* 0x0018 */ uint64_t PfnDatabase;
    /* 0x0020 */ uint64_t PsLoadedModuleList;
    /* 0x0028 */ uint64_t PsActiveProcessHead;
    /* 0x0030 */ uint32_t MachineImageType;
    /* 0x0034 */ uint32_t NumberProcessors;
    /* 0x0038 */ uint32_t BugCheckCode;
    /* 0x003c */ uint32_t __Padding0;
    /* 0x0040 */ std::array<uint64_t, 4> BugCheckCodeParameters;
    /* 0x0060 */ std::array<uint8_t, 32> VersionUser;
    /* 0x0080 */ uint64_t KdDebuggerDataBlock;
    /* 0x0088 */ union DUMP_HEADER64_0 {
        PHYSMEM_DESC PhysicalMemoryBlock;
        std::array<uint8_t, 700> PhysicalMemoryBlockBuffer;
    } u1;
    /* 0x0344 */ uint32_t __Padding1;
    /* 0x0348 */ union CONTEXT_RECORD64_0 {
        CONTEXT_STRUCT ContextRecord;
        std::array<uint8_t, 3000> ContextRecordBuffer;
    } u2;
    /* 0x0f00 */ EXCEPTION_RECORD64 Exception;
    /* 0x0f98 */ DumpType_t DumpType;
    /* 0x0f9c */ uint32_t __Padding2;
    /* 0x0fa0 */ int64_t RequiredDumpSpace;
    /* 0x0fa8 */ int64_t SystemTime;
    /* 0x0fb0 */ std::array<uint8_t, 128> Comment;
    /* 0x1030 */ int64_t SystemUpTime;
    /* 0x1038 */ uint32_t MiniDumpFields;
    /* 0x103c */ uint32_t SecondaryDataState;
    /* 0x1040 */ uint32_t ProductType;
    /* 0x1044 */ uint32_t SuiteMask;
    /* 0x1048 */ uint32_t WriterStatus;
    /* 0x104c */ uint8_t Unused1;
    /* 0x104d */ uint8_t KdSecondaryVersion;
    /* 0x104e */ std::array<uint8_t, 2> Unused;
    /* 0x1050 */ DUMP_FILE_ATTRIBUTES Attributes;
    /* 0x1054 */ uint32_t BootId;
    /* 0x1058 */ std::array<uint8_t, 4008> _reserved0;

    union {
        BMP_HEADER64 BmpHeader;
        KERNEL_RDMP_HEADER64 RdmpHeader;
        FULL_RDMP_HEADER64 FullRdmpHeader;
    } u3;

    bool LooksGood() const {

        //
        // Integrity check the headers.
        //

        if (Signature != ExpectedSignature) {
            return false;
        }

        if (ValidDump != ExpectedValidDump) {
            return false;
        }

        //
        // Make sure it's a dump type we know how to handle.
        //

        switch (DumpType) {
        case DumpType_t::FullDump: {
            if (!u1.PhysicalMemoryBlock.LooksGood()) {
                return false;
            }
            break;
        }
        
        case DumpType_t::CrashDump:
        case DumpType_t::BMPDump: {
            if (!u3.BmpHeader.LooksGood()) {
                return false;
            }
            break;
        }

        case DumpType_t::KernelAndUserMemoryDump:
        case DumpType_t::KernelMemoryDump: {
            if (!u3.RdmpHeader.Hdr.LooksGood()) {
                return false;
            }
            break;
        }

        case DumpType_t::CompleteMemoryDump: {
            if (!u3.FullRdmpHeader.Hdr.LooksGood()) {
                return false;
            }
            break;
        }

        case DumpType_t::MiniDump: {
            return false;
        }

        default: {
            return false;
        }
        }

        //
        // Integrity check the CONTEXT record.
        //

        if (!u2.ContextRecord.LooksGood()) {
            return false;
        }

        return true;
    }
};