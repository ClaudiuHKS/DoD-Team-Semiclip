
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

void memCpy(void* pAddr, const void* pMem, ::size_t Size)
{
    ::allowFullMemAccess(pAddr, Size);
    ::memcpy(pAddr, pMem, Size);
}

const unsigned char* memCmp(const unsigned char* pPos, ::size_t Range, int Ref, ::size_t Pos)
{
    auto pEnd = pPos + Range;
    for (; pPos < pEnd; ++pPos)
        if (*pPos == (unsigned char)'\x83' && *(pPos + Pos) == Ref)
            return pPos;
    return NULL;
}

#ifndef __linux__
bool memCmpChr(const unsigned char* pAddr, const unsigned char* pPattern, const unsigned char* pEnd)
{
    for (auto pChr = pPattern; pChr < pEnd; ++pChr, ++pAddr)
    {
        if (*pChr == *pAddr || *pChr == (unsigned char)'\x2A')
            continue;
        return false;
    }
    return true;
}

const unsigned char* memFindRefChr(const unsigned char* pPos, const unsigned char* pEnd, unsigned char Code, ::size_t Ref, ::size_t Pos)
{
    for (; pPos < pEnd; ++pPos)
        if (*pPos == Code && *(unsigned*)(pPos + Pos) == Ref)
            return pPos;
    return NULL;
}

const unsigned char* findPattern(const unsigned char* pPos, ::size_t Range, const unsigned char* pPattern, ::size_t Size)
{
    auto pPatternEnd = pPattern + Size;
    for (auto pEnd = pPos + Range - Size; pPos < pEnd; ++pPos)
        if (::memCmpChr(pPos, pPattern, pPatternEnd))
            return pPos;
    return NULL;
}

const unsigned char* findStrPush(const unsigned char* pAddr, ::size_t Size, const unsigned char* pString, ::size_t lenZero)
{
    auto pRef = ::findPattern(pAddr, Size, pString, lenZero);
    return ::memFindRefChr(pAddr, pAddr + Size - 5, (unsigned char)'\x68', (::size_t)pRef, 1);
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
