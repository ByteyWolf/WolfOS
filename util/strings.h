#ifndef STRINGS_H
#define STRINGS_H

#include <stdint.h>

int strcmp(const char* a, const char* b);
uint32_t strlen(const char *s);
void *memcpy(void *dst, const void *src, uint32_t n);
void *memset(void *dst,int value,uint32_t n);
void *memmove(void *dst, const void *src, uint32_t n);

#endif
