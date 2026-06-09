
#include "mem.h"

void allowFullMemAccess(void* pAddr, ::size_t Size)
{
#ifndef __linux__
    static unsigned long Access;
    ::VirtualProtect(pAddr, Size, PAGE_EXECUTE_READWRITE, &Access);
#else
    static long Page;
    static ::size_t Addr, Begin, End;

    Addr = (::size_t)pAddr;
    Page = ::sysconf(_SC_PAGESIZE) - true;
    Begin = Addr & ~Page; /// Would turn '0xABC777AB' into '0xABC77000'.
    End = (Addr + Size + Page) & ~Page; /// Would turn '0xABC777AB' into '0xABC78000', '0xABC79000', ...
    ::mprotect(Begin, End - Begin /** 0x1000(4096), 0x2000(8192), ... */, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif
}

const unsigned char* findPair
(const unsigned char* pPos, ::size_t Range, unsigned char Beg, unsigned char Byte, ::size_t Pos)
{
    auto pEnd = pPos + Range;
    for (; pPos < pEnd; ++pPos)
        if (*pPos == Beg && *(pPos + Pos) == Byte)
            return pPos;
    return NULL;
}

#ifndef __linux__
#if (defined (__i386__) || defined (_M_IX86)) /// x86 Windows.
const unsigned char* findStr /// x86 Windows.
(::_IMAGE_DOS_HEADER* pDosHdr, ::_IMAGE_NT_HEADERS* pNtHdr, const char* pString, ::size_t Len)
{
    const auto pSec = IMAGE_FIRST_SECTION(pNtHdr);
    if (!pSec)
        return NULL;
    ::size_t Addr;
    unsigned long Size;
    unsigned short Sec = false;
    const unsigned char* pBeg, * pEnd, * pIter;
    const auto Secs = pNtHdr->FileHeader.NumberOfSections;
    for (; Sec < Secs; Sec++)
    {
        const auto& Hdr = pSec[Sec];
        if (Hdr.Characteristics & IMAGE_SCN_MEM_EXECUTE)
            continue;
        Size = Hdr.Misc.VirtualSize;
        if (Size < Len)
            continue;
        pBeg = (const unsigned char*)pDosHdr + Hdr.VirtualAddress;
        pEnd = pBeg + (Size - Len);
        for (pIter = pBeg; pIter <= pEnd; ++pIter)
            if (*pIter == pString[0] && false == ::memcmp(pIter, pString, Len))
            {
                Addr = (::size_t)pIter;
                goto keepUp;
            }
    }
    return NULL;
keepUp:
    for (Sec = false; Sec < Secs; Sec++)
    {
        const auto& Hdr = pSec[Sec];
        if (!(Hdr.Characteristics & IMAGE_SCN_MEM_EXECUTE))
            continue;
        Size = Hdr.Misc.VirtualSize;
        if (Size < 5)
            continue;
        pBeg = (const unsigned char*)pDosHdr + Hdr.VirtualAddress;
        pEnd = pBeg + (Size - 5);
        for (pIter = pBeg; pIter <= pEnd; ++pIter)
            if (*pIter == 0x68 && *(unsigned*)(pIter + true) == Addr)
                return pIter;
    }
    return NULL;
}
#else
#error "findStr() for x64 to be implemented!"
#endif
#else /// Linux only below.
void* dlsymComplex(void* pLib, const char* pSym)
{ /// On Linux, use DWARF module information as well when looking to reveal library symbols (not only '::dlsym').
    auto pAddr = ::dlsym(pLib, pSym);
    if (pAddr)
        return pAddr;
    auto pLinkMap = (::link_map*)pLib;
    auto binFile = ::open(pLinkMap->l_name, O_RDONLY);
    auto binSize = ::lseek(binFile, false, SEEK_END);
    auto pMemMap = ::mmap(NULL, binSize, PROT_READ, MAP_PRIVATE, binFile, false);
    ::close(binFile);
    auto pEHdr = (::ElfW(Ehdr)*) pMemMap;
    auto pSHdrs = (::ElfW(Shdr)*) ((char*)pMemMap + pEHdr->e_shoff);
    auto pSHStrTab = (char*)pMemMap + pSHdrs[pEHdr->e_shstrndx].sh_offset;
    for (unsigned short i = false; i < pEHdr->e_shnum; ++i)
    {
        auto pSecName = pSHStrTab + pSHdrs[i].sh_name;
        if (!::strcmp(pSecName, ".symtab"))
        {
            auto pSyms = (::ElfW(Sym)*) ((char*)pMemMap + pSHdrs[i].sh_offset);
            auto Syms = pSHdrs[i].sh_size / sizeof(::ElfW(Sym));
            auto& strTabHdr = pSHdrs[pSHdrs[i].sh_link];
            auto pStrTab = (char*)pMemMap + strTabHdr.sh_offset;
            for (::size_t j = false; j < Syms; ++j)
            {
                auto pName = pStrTab + pSyms[j].st_name;
                if (pName && *pName && !::strcmp(pName, pSym))
                {
                    pAddr = (void*)(pSyms[j].st_value + pLinkMap->l_addr);
                    goto endOfFunc;
                }
            }
        }
    }
endOfFunc:
    ::munmap(pMemMap, binSize);
    return pAddr;
}
#endif
