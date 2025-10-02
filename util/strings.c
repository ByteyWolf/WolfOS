#include <stdint.h>
#include "strings.h"
#include "cpuid.h"
#include "../console/console.h"

int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

uint32_t strlen(const char *s) {
    const char *p = s;
    while (*p) {
        p++;
    }
    return (uint32_t)(p - s);
}


// all the fun stuff

// SSE-accelerated
void *memset_sse(void *dst, int value, uint32_t n) {
    uint8_t *ptr = (uint8_t *)dst;
    uint32_t i;

    uint32_t align_bytes = ((uintptr_t)ptr) & 15;
    if (align_bytes) {
        align_bytes = 16 - align_bytes;
        if (align_bytes > n) align_bytes = n;
        for (i = 0; i < align_bytes; i++)
            ptr[i] = (uint8_t)value;
        ptr += align_bytes;
        n -= align_bytes;
    }

    __asm__ volatile (
        "movd   %[val], %%xmm0        \n" // move 32-bit value to xmm0
        "pshufd $0, %%xmm0, %%xmm0   \n" // replicate across 128-bit
        : 
        : [val]"r"(value & 0xFF ? 0x01010101* (value & 0xFF) : 0)
    );

    uint32_t sse_chunks = n / 16;
    for (i = 0; i < sse_chunks; i++) {
        __asm__ volatile (
            "movdqa %%xmm0, (%0) \n"
            :
            : "r"(ptr)
            : "memory"
        );
        ptr += 16;
    }

    uint32_t remaining = n % 16;
    for (i = 0; i < remaining; i++)
        ptr[i] = (uint8_t)value;

    return dst;
}

void *memcpy_sse(void *dest, const void *src, unsigned int n) {
    unsigned char *d = dest;
    const unsigned char *s = src;

    while (((uintptr_t)d & 0xF) && n) {
        *d++ = *s++;
        n--;
    }

    unsigned int chunks = n / 16;
    unsigned int leftover = n % 16;

    putchar('w');
    __asm__ __volatile__ (
        "1:\n"
        "movdqu (%[src]), %%xmm0\n"
        "movdqu %%xmm0, (%[dst])\n"
        "add $16, %[src]\n"
        "add $16, %[dst]\n"
        "dec %[chunks]\n"
        "jnz 1b\n"
        : [dst] "+r" (d), [src] "+r" (s), [chunks] "+r" (chunks)
        :
        : "xmm0", "memory"
    );

    for (unsigned int i = 0; i < leftover; i++)
        *d++ = *s++;

    return dest;
}

void *memmove_sse(void *dest, const void *src, unsigned int n) {
    unsigned char *d = dest;
    const unsigned char *s = src;

    if (d < s) {
        // Forward copy
        while (((uintptr_t)d & 0xF) && n) {
            *d++ = *s++;
            n--;
        }

        unsigned int chunks = n / 16;
        unsigned int leftover = n % 16;

        __asm__ __volatile__ (
            "1:\n"
            "movdqu (%[src]), %%xmm0\n"
            "movdqu %%xmm0, (%[dst])\n"
            "add $16, %[src]\n"
            "add $16, %[dst]\n"
            "dec %[chunks]\n"
            "jnz 1b\n"
            : [dst] "+r" (d), [src] "+r" (s), [chunks] "+r" (chunks)
            :
            : "xmm0", "memory"
        );

        for (unsigned int i = 0; i < leftover; i++)
            *d++ = *s++;
    } else if (d > s) {
        // Backward copy
        d += n;
        s += n;

        // Align end for 16-byte blocks
        while ((uintptr_t)d & 0xF && n) {
            *(--d) = *(--s);
            n--;
        }

        unsigned int chunks = n / 16;
        unsigned int leftover = n % 16;

        __asm__ __volatile__ (
            "1:\n"
            "sub $16, %[src]\n"
            "sub $16, %[dst]\n"
            "movdqu (%[src]), %%xmm0\n"
            "movdqu %%xmm0, (%[dst])\n"
            "dec %[chunks]\n"
            "jnz 1b\n"
            : [dst] "+r" (d), [src] "+r" (s), [chunks] "+r" (chunks)
            :
            : "xmm0", "memory"
        );

        for (unsigned int i = 0; i < leftover; i++)
            *(--d) = *(--s);
    }

    return dest;
}

// STOSB-accelerated
void *memset_fast(void *dst, int val, uint32_t n) {
    uint8_t *d = (uint8_t *)dst;

    if (n == 0) return dst;

    asm volatile(
        "rep stosb"
        : "+D"(d), "+c"(n)
        : "a"(val)
        : "memory"
    );

    return dst;
}
void *memcpy_fast(void *dst, const void *src, uint32_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    if (n == 0) return dst;

    asm volatile(
        "rep movsb"
        : "+D"(d), "+S"(s), "+c"(n)
        :
        : "memory"
    );

    return dst;
}
void *memmove_fast(void *dst, const void *src, uint32_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    if (d == s || n == 0) return dst;

    if (d < s) {
        asm volatile(
            "rep movsb"
            : "+D"(d), "+S"(s), "+c"(n)
            :
            : "memory"
        );
    } else {
        d += n;
        s += n;
        asm volatile(
            "std\n\t"
            "rep movsb\n\t"
            "cld"
            : "+D"(d), "+S"(s), "+c"(n)
            :
            : "memory"
        );
    }

    return dst;
}


void *memcpy(void *dst, const void *src, uint32_t n){
    if (cpu_has_feature(CPUID_FEAT_EDX_SSE)) return memcpy_sse(dst, src, n);
    return memcpy_fast(dst, src, n);
}
void *memset(void *dst,int value,uint32_t n){
    if (cpu_has_feature(CPUID_FEAT_EDX_SSE)) return memset_sse(dst, value, n);
    return memset_fast(dst, value, n);
}
void *memmove(void *dst, const void *src, uint32_t n) {
    if (cpu_has_feature(CPUID_FEAT_EDX_SSE)) return memmove_sse(dst, src, n);
    return memmove_fast(dst, src, n);
}