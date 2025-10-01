#include <stdint.h>
#include "definitions.h"

uint16_t __driver_classes[1] = {0x0C03};
uint32_t __driver_classes_count = 1;

uint16_t __driver_class_priority = 100;

char* __driver_name = "EHCI Driver";
char* __driver_author = "Silly Wolf";

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

    /* compute op_base by adding bytes (caplength is in bytes) */
    volatile struct op_regs *op_base = (volatile struct op_regs *)(mmio + (uint32_t)cap_base->caplength);
    /*LOG("Is this it? ");
    print_ptr(cap_base);
    putchar('\n');
    print_ptr(op_base);
    putchar('\n');*/

    uint16_t cmd = pci_read_word(bus, slot, func, 0x04);
    cmd |= 0b110; // MEM space + Bus master
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
    /*op_base->usbcmd &= ~(1 << 0);
    delay(1000000000);
    //while (!(op_base->usbsts & (1 << USBSTS_HALTED)));*/

    op_base->usbcmd |= 2 | cmd & ~(USBCMD_ASE | USBCMD_PSE);  
    wait(50);
    LOG("Waiting for reset bit clear...");
    while((op_base->usbcmd) & 2);
    LOG("Waiting for halted...");
    while (!(op_base->usbsts & (1 << USBSTS_HALTED)));

    LOG("Allocating data for controller.");
    op_base->periodiclistbase = kamalloc(1024 * sizeof(uint32_t), 4096);

    struct ehci_qh* qh = kamalloc(sizeof(struct ehci_qh), 128);
    qh->qhlp = (uint32_t) qh | 0b01;
    qh->end_char = 1 << 15;
    qh->end_cap = 0;
    qh->current_qtd = 0;
    qh->next_qtd = 1;
    qh->alternate_nextqtd = 0;
    qh->token = 0;
    for (uint32_t i = 0; i < 5; ++i)
	{
		qh->bufptr[i] = 0;
	}
    op_base->asynclistaddr = (uint32_t)qh;

    qh = kamalloc(sizeof(struct ehci_qh), 128);
    qh->qhlp = 0;
	qh->end_char = 0;
	qh->end_cap = 0;
	qh->current_qtd = 0;
	qh->next_qtd = 1;
	qh->alternate_nextqtd = 0;
	qh->token = 0;
    for (uint32_t i = 0; i < 5; ++i)
		qh->bufptr[i] = 0;
    for (uint32_t i = 0; i < 1024; ++i)
		op_base->periodiclistbase[i] = 0b01 | (uint32_t)qh;

    print("[EHCI] Turning off BIOS legacy mode... ");
    uint16_t eecp = ((cap_base->hccparams & 0xFF00) >> 8);
    if (eecp > 0x40) {
        uint32_t legsup = pci_read_dword(bus, slot, func, eecp + USBLEGSUP);
        if (legsup & USBLEGSUP_HC_BIOS) {
            print("USB controller indeed owned by BIOS. Stealing... ");
            pci_write_dword(bus, slot, func, eecp + USBLEGSUP, legsup | USBLEGSUP_HC_OS);
            while(1)
			{
				legsup = pci_read_dword(bus, slot, func, eecp + USBLEGSUP);
				if (~legsup & USBLEGSUP_HC_BIOS && legsup & USBLEGSUP_HC_OS)
				{
					break;
				}
			}
        }
    }

    print("done.\n");
    op_base->usbintr = 0;
    op_base->frindex = 0;
    // periodic and async base already set!
    op_base->ctrldssegment = 0;
    op_base->usbsts = 0;
    op_base->usbcmd = (8 << 16 | USBCMD_ASE | USBCMD_PSE | USBCMD_RUN);
    LOG("Waiting for the controller to start...");
    while ((op_base->usbsts & (1 << USBSTS_HALTED)));
    op_base->configflag = 1;

    uint8_t num_ports = hcsparams & 0xF;
    LOG("Ports available: ");
    print_u8(num_ports);
    putchar('\n');

    for (int i = 0; i < num_ports; i++) {
        volatile uint32_t *port = (op_base->ports) + i;
        if (*port & (1 << PORT_CONNECTED)) {
            print("Device connected on port "); print_u8(i); print("... ");
            
            *port |= (1<<12)|(1<<20);
            wait(100);
            *port |= (1 << PORT_RESET);
            wait(50);
            *port &= ~(1 << PORT_RESET);

            uint32_t status = 0;
            for (uint32_t j = 0; j < 10; ++j)
            {
                wait(10);
                status = *port;

                if (!(status & (1 << PORT_CONNECTED)))
                    break;

                if (status & ((1 << PORT_ENABLED_CHANGE) | (1 << PORT_CONNECTED_CHANGE)))
                {
                    *port |= (1 << PORT_ENABLED_CHANGE) | (1 << PORT_CONNECTED_CHANGE); // clear change flags
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
            
            /*char* ctrl_qh = build_queue_head();
            configure_queue_head_basic(ctrl_qh, 0, 0, 1, 0);
            prelim_get_device_desc(ctrl_qh, (uint32_t)op_base, i);
            kfree(ctrl_qh);*/

        }
    }


    return 0;
}
