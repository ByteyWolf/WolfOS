#include <stdint.h>
#include "definitions.h"

uint16_t __driver_classes[1] = {0x0101};
uint32_t __driver_classes_count = 1;

uint16_t __driver_class_priority = 100;

char* __driver_name = "IDE Driver";
char* __driver_author = "Bytey Wolf";

char* ide_buffer = 0;
volatile unsigned static char ide_irq_invoked = 0;
unsigned static char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int init() {
    print("Silly Wolf IDE Driver version 0.0\n");
    ide_buffer = kmalloc(2048);
    if (!ide_buffer) {
        FAIL("Not enough memory");
    }
    return 0;
}

int bind_device_pci(uint8_t bus, uint8_t slot, uint8_t func) {
    LOG("IDE stub");

    return 0;
}
