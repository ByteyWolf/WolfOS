#include <stdint.h>
#include "cpuid.h"

char cpu_vendor[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
uint32_t cpu_capabilities = 0;
uint32_t cpu_capabilities_extended = 0;

int cpu_has_cpuid(void)
{
    uint32_t supported;
    __asm__ volatile (
        "pushfl\n\t"               // save original EFLAGS
        "pushfl\n\t"               // copy
        "popl %%eax\n\t"           // eax = original EFLAGS
        "movl %%eax, %%ecx\n\t"    // copy to ecx
        "xorl $0x200000, %%eax\n\t"// toggle ID bit
        "pushl %%eax\n\t"          // push modified flags
        "popfl\n\t"                // load modified flags
        "pushfl\n\t"               // push new flags
        "popl %%eax\n\t"           // eax = new flags
        "xorl %%ecx, %%eax\n\t"    // eax = difference
        "andl $0x200000, %%eax"    // mask ID bit
        : "=a"(supported)
        :
        : "ecx"
    );
    return supported != 0;
}


void load_cpuid() {
    // load certain things like capabilities and cpu name
    if (!cpu_has_cpuid()) return;
    uint32_t eax, ebx, ecx, edx;

    __asm__ volatile (
        "mov $0, %%eax\n\t"
        "cpuid\n\t"
        : "=b"(ebx), "=d"(edx), "=c"(ecx), "=a"(eax)
        :
        :
    );

    ((uint32_t*)cpu_vendor)[0] = ebx;
    ((uint32_t*)cpu_vendor)[1] = edx;
    ((uint32_t*)cpu_vendor)[2] = ecx;
    cpu_vendor[12] = '\0';    

    __asm__ volatile (
        "mov $1, %%eax\n\t"
        "cpuid\n\t"
        : "=c"(ecx), "=d"(edx)
        :
        : "eax", "ebx"
    );
    cpu_capabilities = edx;
    cpu_capabilities_extended = ecx;
    if (cpu_has_feature(CPUID_FEAT_EDX_SSE)) {enable_sse();}
}

char* get_cpu_vendor() {
    return cpu_vendor;
}

uint8_t cpu_has_feature(uint32_t feature) {
    return (cpu_capabilities & feature) != 0;
}

void enable_sse() {
    uint32_t eax;

    /* CR0: clear EM (bit2), set MP (bit1), clear TS (bit3) */
    asm volatile("mov %%cr0, %0" : "=r"(eax));
    eax &= ~(1u << 2);  /* clear EM */
    eax |=  (1u << 1);  /* set MP */
    eax &= ~(1u << 3);  /* clear TS */
    asm volatile("mov %0, %%cr0" :: "r"(eax));

    /* CR4: set OSFXSR (bit9) and OSXMMEXCPT (bit10) */
    asm volatile("mov %%cr4, %0" : "=r"(eax));
    eax |= (1u << 9) | (1u << 10);
    asm volatile("mov %0, %%cr4" :: "r"(eax));

    /* Initialize FPU and MXCSR */
    asm volatile("fninit");
    uint32_t mx = 0x1F80;
    asm volatile("ldmxcsr %0" :: "m"(mx));
    asm volatile("pxor %%xmm0, %%xmm0" ::: "xmm0");
}
