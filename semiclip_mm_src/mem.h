
#ifndef _MEMORY_H_
#define _MEMORY_H_

#ifndef __linux__
#include <Windows.h>
#else
#include <string.h>
#include <fcntl.h>
#include <link.h>
#include <unistd.h>
#include <sys/mman.h>
#endif

void allowFullMemAccess(void*, ::size_t);
const unsigned char* findPair(const unsigned char*, ::size_t, unsigned char, unsigned char, ::size_t);

#ifndef __linux__
const unsigned char* findStr(::_IMAGE_DOS_HEADER*, ::_IMAGE_NT_HEADERS*, const char*, ::size_t);
#else
void* dlsymComplex(void*, const char*);
#endif

#endif
