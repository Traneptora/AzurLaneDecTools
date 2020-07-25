#ifndef _COMPAT_H
#define _COMPAT_H

#ifdef __CPLUSPLUS
extern "C" {
#endif // __CPLUSPLUS

#ifdef _WIN32
#include <stdlib.h>
#include <intrin.h>
#else // _WIN32
unsigned long _byteswap_ulong(unsigned long x);
uint32_t _rotl(uint32_t x, uint32_t n);
unsigned char _BitScanReverse(unsigned long *index, unsigned long mask);
#endif // _WIN32

#ifdef __CPLUSPLUS
}
#endif // __CPLUSPLUS

#endif // _COMPAT_H
