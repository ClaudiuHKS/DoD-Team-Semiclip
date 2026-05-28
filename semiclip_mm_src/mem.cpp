
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

const unsigned char* findMem(const unsigned char* pPos, ::size_t Range, ::size_t Ref, ::size_t Pos)
{
    auto pEnd = pPos + Range;
    for (; pPos < pEnd; ++pPos)
        if (*pPos == (unsigned char)'\x83' && *(pPos + Pos) == Ref)
            return pPos;
    return NULL;
}

#ifndef __linux__
const unsigned char* findStr
(::_IMAGE_DOS_HEADER* pDosHdr, ::_IMAGE_NT_HEADERS* pNtHdr, const char* pString, ::size_t Len)
{
    auto pSec = IMAGE_FIRST_SECTION(pNtHdr);
    if (!pSec)
        return NULL;
    unsigned long Size;
    unsigned short Sec;
    ::size_t strAddr, Iter;
    const unsigned char* pBeg;
    auto Secs = pNtHdr->FileHeader.NumberOfSections;
    for (Sec = false; Sec < Secs; Sec++)
    {
        pBeg = (const unsigned char*)pDosHdr + pSec[Sec].VirtualAddress;
        Size = pSec[Sec].Misc.VirtualSize;
        for (Iter = false; Iter < Size - Len; Iter++)
            if (false == ::memcmp(pBeg + Iter, pString, Len))
            {
                strAddr = (::size_t)(pBeg + Iter);
                goto keepUp;
            }
    }
    return NULL;
keepUp:
    ::size_t Addr;
    const unsigned char* pPos;
    for (Sec = false; Sec < Secs; Sec++)
    {
        if (!(pSec[Sec].Characteristics & IMAGE_SCN_MEM_EXECUTE))
            continue;
        pBeg = (const unsigned char*)pDosHdr + pSec[Sec].VirtualAddress;
        Size = pSec[Sec].Misc.VirtualSize;
        for (Iter = false; Iter < Size - 5; Iter++)
        {
            pPos = pBeg + Iter;
            if (pPos[0] == 0x68)
            {
                Addr = *(::size_t*)(pPos + true);
                if (Addr == strAddr)
                    return pPos;
            }
        }
    }
    return NULL;
}
#else
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
