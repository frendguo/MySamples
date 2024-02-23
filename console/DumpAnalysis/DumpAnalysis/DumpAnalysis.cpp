#include "KDmpStruct.h"
#include "FileMap.h"
#include "Util.h"

#include <optional>
#include <iostream>
#include <wchar.h>
#include <unordered_map>
#include <DbgHelp.h>

using physmem_t = std::unordered_map<uint64_t, const uint8_t*>;
LARGE_INTEGER FileSize = { 0 };


bool InBounds(PVOID ViewBase_, const void* Ptr, const size_t Size) {
    const uint64_t FileSize_ = Page::Align(FileSize.QuadPart) + Page::Size;
    const uint8_t* ViewEnd = (uint8_t*)ViewBase_ + FileSize_;
    const uint8_t* PtrEnd = (uint8_t*)Ptr + Size;
    return PtrEnd > Ptr && ViewEnd > ViewBase_ && Ptr >= ViewBase_ &&
        PtrEnd < ViewEnd;
}

physmem_t BuildPhysicalMemoryFromDump(HEADER64* DmpHdr_, PVOID mapAddress, const DumpType_t Type) {
    uint64_t FirstPageOffset = 0;
    uint8_t* Page = nullptr;
    uint64_t MetadataSize = 0;
    uint8_t* Bitmap = nullptr;
    physmem_t Physmem_;

    switch (Type) {
    case DumpType_t::KernelMemoryDump:
    case DumpType_t::KernelAndUserMemoryDump: {
        FirstPageOffset = DmpHdr_->u3.RdmpHeader.Hdr.FirstPageOffset;
        Page = (uint8_t*)DmpHdr_ + FirstPageOffset;
        MetadataSize = DmpHdr_->u3.RdmpHeader.Hdr.MetadataSize;
        Bitmap = DmpHdr_->u3.RdmpHeader.Bitmap.data();
        break;
    }

    case DumpType_t::CrashDump:
    case DumpType_t::CompleteMemoryDump: {
        FirstPageOffset = DmpHdr_->u3.RdmpHeader.Hdr.FirstPageOffset;
        Page = (uint8_t*)DmpHdr_ + FirstPageOffset;
        MetadataSize = DmpHdr_->u3.FullRdmpHeader.Hdr.MetadataSize;
        Bitmap = DmpHdr_->u3.FullRdmpHeader.Bitmap.data();
        break;
    }

    default: {
        return Physmem_;
    }
    }

    if (!FirstPageOffset || !Page || !MetadataSize || !Bitmap) {
        return Physmem_;
    }

    auto IsPageInBounds = [&](const uint8_t* Ptr) {
        return InBounds(mapAddress, Ptr, Page::Size);
        };

    if (!IsPageInBounds(Page)) {
        return Physmem_;
    }

    struct PfnRange {
        uint64_t PageFileNumber;
        uint64_t NumberOfPages;
    };

    for (uint64_t Offset = 0; Offset < MetadataSize;
        Offset += sizeof(PfnRange)) {
        const PfnRange& Entry = (PfnRange&)Bitmap[Offset];
        if (!InBounds(mapAddress, &Entry, sizeof(Entry))) {
            return Physmem_;
        }

        const uint64_t Pfn = Entry.PageFileNumber;
        if (!Pfn) {
            break;
        }

        for (uint64_t PageIdx = 0; PageIdx < Entry.NumberOfPages; PageIdx++) {
            if (!IsPageInBounds(Page)) {
                return Physmem_;
            }

            const uint64_t Pa = (Pfn * Page::Size) + (PageIdx * Page::Size);
            Physmem_.try_emplace(Pa, Page);
            Page += Page::Size;
        }
    }

    return Physmem_;
}

physmem_t BuildPhysmemBMPDump(HEADER64* DmpHdr_) {
    physmem_t Physmem_;
    const uint8_t* Page = (uint8_t*)DmpHdr_ + DmpHdr_->u3.BmpHeader.FirstPage;
    const uint64_t BitmapSize = DmpHdr_->u3.BmpHeader.Pages / 8;
    const uint8_t* Bitmap = DmpHdr_->u3.BmpHeader.Bitmap.data();

    //
    // Walk the bitmap byte per byte.
    //

    for (uint64_t BitmapIdx = 0; BitmapIdx < BitmapSize; BitmapIdx++) {

        //
        // Now walk the bits of the current byte.
        //

        const uint8_t Byte = Bitmap[BitmapIdx];
        for (uint8_t BitIdx = 0; BitIdx < 8; BitIdx++) {

            //
            // If the bit is not set we just skip to the next.
            //

            const bool BitSet = ((Byte >> BitIdx) & 1) == 1;
            if (!BitSet) {
                continue;
            }

            //
            // If the bit is one we add the page to the physmem.
            //

            const uint64_t Pfn = (BitmapIdx * 8) + BitIdx;
            const uint64_t Pa = Pfn * Page::Size;
            Physmem_.try_emplace(Pa, Page);
            Page += Page::Size;
        }
    }

    return Physmem_;
}

const uint8_t* GetPhysicalPage(physmem_t physmem, const uint64_t PhysicalAddress) {

    //
    // Attempt to find the physical address.
    //

    const auto& Pair = physmem.find(PhysicalAddress);

    //
    // If it doesn't exist then return nullptr.
    //

    if (Pair == physmem.end()) {
        return nullptr;
    }

    //
    // Otherwise we return a pointer to the content of the page.
    //

    return Pair->second;
}

uint64_t PhyRead8(physmem_t physmem, const uint64_t PhysicalAddress) {

    //
    // Get the physical page and read from the offset.
    //

    const uint8_t* PhysicalPage = GetPhysicalPage(physmem, Page::Align(PhysicalAddress));

    if (!PhysicalPage) {
        printf("Internal page table parsing failed!\n");
        return 0;
    }

    const uint64_t* Ptr =
        (uint64_t*)(PhysicalPage + Page::Offset(PhysicalAddress));
    return *Ptr;
}

uint64_t TranslateVirToPhy(HEADER64* header, physmem_t physmem,
                           const uint64_t VirtualAddress,
                           const uint64_t DirectoryTableBase = 0) {
    auto directoryTablebase = header->DirectoryTableBase;
    uint64_t LocalDTB = Page::Align(directoryTablebase);
    if (DirectoryTableBase != 0) {
        LocalDTB = Page::Align(DirectoryTableBase);
    }

    const VIRTUAL_ADDRESS GuestAddress(VirtualAddress);
    const MMPTE_HARDWARE Pml4(LocalDTB);
    const uint64_t Pml4Base = Pml4.u.PageFrameNumber * Page::Size;
    const uint64_t Pml4eGpa = Pml4Base + GuestAddress.u.Pml4Index * 8;
    const MMPTE_HARDWARE Pml4e(PhyRead8(physmem, Pml4eGpa));
    if (!Pml4e.u.Present) {
        return 0;
    }

    const uint64_t PdptBase = Pml4e.u.PageFrameNumber * Page::Size;
    const uint64_t PdpteGpa = PdptBase + GuestAddress.u.PdPtIndex * 8;
    const MMPTE_HARDWARE Pdpte(PhyRead8(physmem, PdpteGpa));
    if (!Pdpte.u.Present) {
        return 0;
    }

    //
    // huge pages:
    // 7 (PS) - Page size; must be 1 (otherwise, this entry references a page
    // directory; see Table 4-1
    //

    const uint64_t PdBase = Pdpte.u.PageFrameNumber * Page::Size;
    if (Pdpte.u.LargePage) {
        return PdBase + (VirtualAddress & 0x3fff'ffff);
    }

    const uint64_t PdeGpa = PdBase + GuestAddress.u.PdIndex * 8;
    const MMPTE_HARDWARE Pde(PhyRead8(physmem, PdeGpa));
    if (!Pde.u.Present) {
        return 0;
    }

    //
    // large pages:
    // 7 (PS) - Page size; must be 1 (otherwise, this entry references a page
    // table; see Table 4-18
    //

    const uint64_t PtBase = Pde.u.PageFrameNumber * Page::Size;
    if (Pde.u.LargePage) {
        return PtBase + (VirtualAddress & 0x1f'ffff);
    }

    const uint64_t PteGpa = PtBase + GuestAddress.u.PtIndex * 8;
    const MMPTE_HARDWARE Pte(PhyRead8(physmem, PteGpa));
    if (!Pte.u.Present) {
        return 0;
    }

    const uint64_t PageBase = Pte.u.PageFrameNumber * Page::Size;
    return PageBase + GuestAddress.u.Offset;
}

const uint8_t* GetVirtualPage(HEADER64* header, physmem_t physmem, const uint64_t VirtualAddress,
    const uint64_t DirectoryTableBase = 0)  {

    //
    // First remove offset and translate the virtual address.
    //

    const auto PhysicalAddress =
        TranslateVirToPhy(header, physmem, Page::Align(VirtualAddress), DirectoryTableBase);

    if (!PhysicalAddress) {
        return nullptr;
    }

    //
    // Then get the physical page.
    //

    return GetPhysicalPage(physmem , PhysicalAddress);
}

// 读取和分析minidump文件
void AnalyzeMinidump(LPCWSTR minidumpPath) {
    HANDLE hFile = CreateFile(minidumpPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "Cannot open minidump file. error code is " << GetLastError() << std::endl;
        return;
    }

    HANDLE hMapFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapFile == NULL) {
        std::cout << "Cannot create file mapping. error code is " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return;
    }

    PVOID mapAddress = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
    if (mapAddress == NULL) {
        std::cout << "Cannot map view of file. error code is " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        CloseHandle(hFile);
        return;
    }

    auto dumpHeader = (HEADER64*)mapAddress;
    if (!dumpHeader->LooksGood()) {
        printf("Invalid minidump file.\n");
        std::cout << "Invalid minidump file." << std::endl;
    }
    else {
        std::wcout << "Dump file path: " << minidumpPath << std::endl;
        std::cout << "DumpType : " << (int)dumpHeader->DumpType << std::endl;
        std::cout << "MajorVersion : " << dumpHeader->MajorVersion << std::endl;
        std::cout << "MinorVersion : " << dumpHeader->MinorVersion << std::endl;
        std::cout << "ProcessCounter : " << dumpHeader->NumberProcessors << std::endl;
        std::cout << std::hex;
        std::cout << "MachineType : " << dumpHeader->MachineImageType << std::endl;
        std::cout << "Crash time: " << ConvertFileTimeIntToSystemTime(dumpHeader->SystemTime) << std::endl;
        std::cout << "System UpTime: " << FormatSystemUpTime(dumpHeader->SystemUpTime) << std::endl;
        std::cout << "BugCheckCode : 0x" << dumpHeader->BugCheckCode << std::endl;
        std::cout << "dumpHeader->BugCheckCodeParameters[0] : " << dumpHeader->BugCheckCodeParameters[0] << std::endl;
        std::cout << "dumpHeader->BugCheckCodeParameters[1] : " << dumpHeader->BugCheckCodeParameters[1] << std::endl;
        std::cout << "dumpHeader->BugCheckCodeParameters[2] : " << dumpHeader->BugCheckCodeParameters[2] << std::endl;
        std::cout << "dumpHeader->BugCheckCodeParameters[3] : " << dumpHeader->BugCheckCodeParameters[3] << std::endl;
        std::cout << std::oct;
    }

    UnmapViewOfFile(mapAddress);
    CloseHandle(hMapFile);
    CloseHandle(hFile);
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <MinidumpFile>" << std::endl;
        return 1;
    }

    AnalyzeMinidump(argv[1]);

    system("pause");
    return 0;
}
