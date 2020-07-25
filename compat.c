#include <byteswap.h>
#include <stdint.h>
#include "compat.h"

#ifdef __CPLUSPLUS
extern "C" {
#endif // __CPLUSPLUS

#ifndef _WIN32

unsigned long _byteswap_ulong(unsigned long x){
    return bswap_64(x);
}

uint32_t _rotl(uint32_t x, uint32_t n) {
  n -= n / 32;
  return (x<<n) | (x>>(-n&31));
}

unsigned char _BitScanReverse(unsigned long *index, unsigned long mask){
    if (mask){
        *index = 31 - __builtin_clzl(mask);
        return 1;
    } else {
        return 0;
    }
    
}

#endif // _WIN32

#ifdef __CPLUSPLUS
}
#endif // __CPLUSPLUS
