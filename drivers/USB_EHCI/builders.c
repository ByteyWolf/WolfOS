#include "definitions.h"
#include <stdint.h>

int get_bit(const unsigned char *data, uint32_t bit_offset) {
    uint32_t byte_index = bit_offset / 8;
    uint32_t bit_index = bit_offset % 8;
    return (data[byte_index] >> bit_index) & 1;
}

void set_bit(unsigned char *data, uint32_t bit_offset, int value) {
    uint32_t byte_index = bit_offset / 8;
    uint32_t bit_index = bit_offset % 8;
    if (value)
        data[byte_index] |= (1 << bit_index);
    else
        data[byte_index] &= ~(1 << bit_index);
}

void print_u8(uint8_t n) {
    if (n >= 100) {
        putchar('0' + n / 100);
        n %= 100;
        putchar('0' + n / 10);
        n %= 10;
        putchar('0' + n);
    } else if (n >= 10) {
        putchar('0' + n / 10);
        n %= 10;
        putchar('0' + n);
    } else {
        putchar('0' + n);
    }
}

struct ehci_qtd* qtd_builder(struct ehci_qtd* prev_qtd, uint8_t toggle, uint8_t packet_type, uint8_t packet_size, void* buffer_ptr) {
    struct ehci_qtd* qtd = kamalloc(sizeof(struct ehci_qtd), 128);
}