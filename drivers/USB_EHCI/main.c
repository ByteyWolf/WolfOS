#include <stdint.h>
#include "definitions.h"

uint16_t __driver_classes[1] = {0x0C03};
uint32_t __driver_classes_count = 1;

uint16_t __driver_class_priority = 100;

char* __driver_name = "EHCI Driver";
char* __driver_author = "Bytey Wolf";

int init() {
    print("Silly Wolf EHCI Driver version 1.0\n");
    return 0;
}

int bind_device_pci(uint8_t bus, uint8_t slot, uint8_t func) {
    LOG("Discovered new device, binding!");
    uint8_t progif = pci_read_byte(bus, slot, func, 0x09);
    if (progif != 0x20) { FAIL("This isn't an EHCI controller!"); }

    uint32_t bar = pci_read_dword(bus, slot, func, 0x10) & 0xFFFFFFF0;
    volatile uint8_t *mmio = (volatile uint8_t *)(uintptr_t)bar;
    volatile struct cap_regs *cap_base = (volatile struct cap_regs *)mmio;

    volatile struct op_regs *op_base = (volatile struct op_regs *)(mmio + (uint32_t)cap_base->caplength);

    uint16_t cmd = pci_read_word(bus, slot, func, 0x04);
    cmd |= 0b110;
    pci_write_word(bus, slot, func, 0x04, cmd);

    print("CAPLENGTH = ");
    putchar('0' + cap_base->caplength/16);
    putchar('0' + cap_base->caplength%16);
    putchar('\n');

    uint32_t hcsparams = cap_base->hcsparams;
    uint32_t hccparams = cap_base->hccparams;
    print("HCSPARAMS = "); print_ptr((void*)hcsparams); putchar('\n');
    print("HCCPARAMS = "); print_ptr((void*)hccparams); putchar('\n');

    LOG("Resetting controller.");

    op_base->usbcmd = (op_base->usbcmd & ~(USBCMD_ASE | USBCMD_PSE)) | 2;
    wait(50);
    LOG("Waiting for reset bit clear...");
    while((op_base->usbcmd) & 2);
    LOG("Waiting for halted...");
    while (!(op_base->usbsts & (1 << USBSTS_HALTED)));

    LOG("Allocating data for controller.");
    uint32_t *periodic_list = (uint32_t *)kamalloc(1024 * sizeof(uint32_t), 4096);
    op_base->periodiclistbase = periodic_list;

    struct ehci_qh* async_qh = kamalloc(sizeof(struct ehci_qh), 128);
    async_qh->qhlp = (uint32_t)async_qh | 0b01;
    async_qh->end_char = 1 << 15;
    async_qh->end_cap = 0;
    async_qh->current_qtd = 0;
    async_qh->next_qtd = 1;
    async_qh->alternate_nextqtd = 0;
    async_qh->token = 0;
    for (uint32_t i = 0; i < 5; ++i) {
        async_qh->bufptr[i] = 0;
    }
    op_base->asynclistaddr = (uint32_t)async_qh;

    struct ehci_qh* periodic_qh = kamalloc(sizeof(struct ehci_qh), 128);
    periodic_qh->qhlp = 0;
    periodic_qh->end_char = 0;
    periodic_qh->end_cap = 0;
    periodic_qh->current_qtd = 0;
    periodic_qh->next_qtd = 1;
    periodic_qh->alternate_nextqtd = 0;
    periodic_qh->token = 0;
    for (uint32_t i = 0; i < 5; ++i) {
        periodic_qh->bufptr[i] = 0;
    }
    for (uint32_t i = 0; i < 1024; ++i) {
        periodic_list[i] = 0b01 | (uint32_t)periodic_qh;
    }

    print("[EHCI] Turning off BIOS legacy mode... ");
    uint16_t eecp = ((cap_base->hccparams & 0xFF00) >> 8);
    if (eecp > 0x40) {
        uint32_t legsup = pci_read_dword(bus, slot, func, eecp + USBLEGSUP);
        if (legsup & USBLEGSUP_HC_BIOS) {
            print("USB controller indeed owned by BIOS. Stealing... ");
            pci_write_dword(bus, slot, func, eecp + USBLEGSUP, legsup | USBLEGSUP_HC_OS);
            while(1) {
                legsup = pci_read_dword(bus, slot, func, eecp + USBLEGSUP);
                if (~legsup & USBLEGSUP_HC_BIOS && legsup & USBLEGSUP_HC_OS) {
                    break;
                }
            }
        }
    }

    print("done.\n");
    op_base->usbintr = 0;
    op_base->frindex = 0;
    op_base->ctrldssegment = 0;
    op_base->usbsts = 0x3F;
    op_base->usbcmd = (8 << 16) | USBCMD_ASE | USBCMD_PSE | USBCMD_RUN;
    LOG("Waiting for the controller to start...");
    while ((op_base->usbsts & (1 << USBSTS_HALTED)));
    op_base->configflag = 1;

    uint8_t num_ports = hcsparams & 0xF;
    LOG("Ports available: ");
    print_uint32(num_ports, 10, "0123456789");
    putchar('\n');

    if (hcsparams & (1 << 4)) {
        for (int i = 0; i < num_ports; i++) {
            op_base->ports[i] |= (1 << 12);
        }
        wait(20);
    }

    for (int i = 0; i < num_ports; i++) {
        volatile uint32_t *port = (op_base->ports) + i;
        if (*port & (1 << PORT_CONNECTED)) {
            print("Device connected on port "); print_uint32(i, 10, "0123456789"); print("... ");

            *port = (*port & ~0x2A) | 0x1540;
            wait(10);
            *port |= (1 << PORT_RESET);
            wait(50);
            *port &= ~(1 << PORT_RESET);

            uint32_t status = 0;
            for (uint32_t j = 0; j < 10; ++j) {
                wait(10);
                status = *port;

                if (!(status & (1 << PORT_CONNECTED)))
                    break;

                if (status & ((1 << PORT_ENABLED_CHANGE) | (1 << PORT_CONNECTED_CHANGE))) {
                    *port |= (1 << PORT_ENABLED_CHANGE) | (1 << PORT_CONNECTED_CHANGE);
                    continue;
                }
                if (status & (1 << PORT_ENABLED))
                    break;
            }

            if (status & (1 << PORT_ENABLED)) {
                print("HS device powered on!\n");
            } else {
                print("\xFCThe device is not enabled!\xF7\n");
                continue;
            }
        }
    }

    return 0;
}
