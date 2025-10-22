#include <stdint.h>
#include "definitions.h"

uint16_t __driver_classes[1] = {0x0101};
uint32_t __driver_classes_count = 1;

uint16_t __driver_class_priority = 100;

char* __driver_name = "IDE Driver";
char* __driver_author = "Bytey Wolf";

char* ide_buffer = 0;
static volatile unsigned char ide_irq_invoked = 0;
static unsigned char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

struct ide_channel* ATA_PRIMARY = 0;
struct ide_channel* ATA_SECONDARY = 0;
struct ide_device* ide_devices[4] = {0};

int init_internal(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t bar0 = 0, bar1 = 0, bar2 = 0, bar3 = 0, bar4 = 0;
    ide_buffer = kmalloc(2048);
    ATA_PRIMARY = kmalloc(sizeof(struct ide_channel));
    ATA_SECONDARY = kmalloc(sizeof(struct ide_channel));
    for (int i=0; i<4; i++) {
        ide_devices[i] = kmalloc(sizeof(struct ide_device));
    }
    if (!ide_buffer || !ATA_PRIMARY || !ATA_SECONDARY) {
        FAIL("Not enough memory");
    }
    if (bus == 0 && slot == 0 && func == 0) {
        bar0 = 0x1F0;
        bar1 = 0x3F6;
        bar2 = 0x170;
        bar3 = 0x376;
        bar4 = 0x000;

    }
    //LOG("IDE stub");
    ATA_PRIMARY->base = bar0;
    ATA_PRIMARY->ctrl = bar1;
    ATA_SECONDARY->base = bar2;
    ATA_SECONDARY->ctrl = bar3;
    ATA_PRIMARY->bmide = bar4;
    ATA_SECONDARY->bmide = bar4 + 8;
    
    // are you sure?
    ide_write(ATA_PRIMARY  , ATA_REG_CONTROL, 2);
    ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    int count = 0;
    for (int chnl = 0; chnl<2; chnl++) {
        for (int drv = 0; drv < 2; drv++) {
            struct ide_channel* chnlp = chnl == 0 ? ATA_PRIMARY : ATA_SECONDARY;
            uint8_t err = 0, type = IDE_ATA, status;
            ide_devices[count]->reserved = 0;
            ide_write(chnlp, ATA_REG_HDDEVSEL, 0xA0 | (drv << 4));
            wait(1);
            ide_write(chnlp, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            wait(1);

            if (ide_read(chnlp, ATA_REG_STATUS) == 0) continue;
            while(1) {
                status = ide_read(chnlp, ATA_REG_STATUS);
                if ((status & ATA_SR_ERR)) {err = 1; break;}
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break;
            }

            if (err != 0) {
                uint8_t cl = ide_read(chnlp, ATA_REG_LBA1);
                uint8_t ch = ide_read(chnlp, ATA_REG_LBA2);

                if ((cl == 0x14 && ch == 0xEB) || (cl == 0x69 && ch == 0x96))
                    type = IDE_ATAPI;
                else
                    continue;

                ide_write(chnlp, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                wait(1);
            }
            ide_read_buffer(chnlp, ATA_REG_DATA, (uint32_t)ide_buffer, 128);

            ide_devices[count]->reserved     = 1;
            ide_devices[count]->type         = type;
            ide_devices[count]->channel      = chnl;
            ide_devices[count]->drive        = drv;
            ide_devices[count]->signature    = *((unsigned short *)(ide_buffer + ATA_IDENT_DEVICETYPE));
            ide_devices[count]->capabilities = *((unsigned short *)(ide_buffer + ATA_IDENT_CAPABILITIES));
            ide_devices[count]->command_sets  = *((unsigned int *)(ide_buffer + ATA_IDENT_COMMANDSETS));

            if (ide_devices[count]->command_sets & (1 << 26))
                ide_devices[count]->size_sectors   = *((unsigned int *)(ide_buffer + ATA_IDENT_MAX_LBA_EXT));
            else
                ide_devices[count]->size_sectors   = *((unsigned int *)(ide_buffer + ATA_IDENT_MAX_LBA));

            for(int k = 0; k < 40; k += 2) {
                ide_devices[count]->model[k] = ide_buffer[ATA_IDENT_MODEL + k + 1];
                ide_devices[count]->model[k + 1] = ide_buffer[ATA_IDENT_MODEL + k];}
            ide_devices[count]->model[40] = 0;

            count++;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (ide_devices[i]->reserved == 1) {
            print("\xFEIDE Channel ");
            print_uint32(ide_devices[i]->channel, 3, "012");
            print(" drive ");
            print_uint32(ide_devices[i]->drive, 3, "012");
            print(": \xF7");
            print(ide_devices[i]->model);
            print("\nSize (KB): ");
            print_uint32(ide_devices[i]->size_sectors / 2, 10, "0123456789");
            putchar('\n');
        }
    }
    

    return 0;
}

int init() {
    print("Silly Wolf IDE Driver version 0.0\n");
    
    return init_internal(0, 0, 0);
}

int bind_device_pci(uint8_t bus, uint8_t slot, uint8_t func) {
    FAIL("no pci ide supported currently...");
}