#include "../console/console.h"
#include "../util/earlyutil.h"
#include "../mm/kalloc.h"
#include "../progman/elf.h"
#include "pci.h"
#include "pci_names.h"

#include "drivermodel.h"

uint32_t drivers_loaded_total = 0;

struct device* device_tree_base = 0;
struct device_driver* driver_tree_base = 0;

struct device_driver* find_driver(uint8_t class, uint8_t subclass, uint16_t vid, uint16_t pid, uint16_t max_priority) {
    uint16_t classdata = (class << 8) + subclass;
    uint32_t vidpid = (vid << 16) + pid;

    struct device_driver* generic_found = 0;
    uint16_t generic_priority = 0;
    struct device_driver* crt_driver = driver_tree_base;
    while (crt_driver) {
        for (uint32_t class = 0; class<crt_driver->driver_classes_count; class++) {
            if (crt_driver->driver_classes[class] == classdata && crt_driver->driver_class_priority >= generic_priority && crt_driver->driver_class_priority < max_priority) {
                generic_found = crt_driver;
                generic_priority = crt_driver->driver_class_priority;
            }
        }
        for (uint32_t vidpidid = 0; vidpidid<crt_driver->driver_vidpid_count; vidpidid++) {
            if (crt_driver->driver_vidpid[vidpidid] == vidpid) return crt_driver;
        }
        crt_driver = crt_driver->next_drv;
    }
    return generic_found;
}

struct device_driver* register_driver_i(struct driver_init_passport* init_passport) {
    driver_init_v1 init_function = get_elf_symbol(init_passport, "init");
    if (init_function == 0) {
        printf("\xFC""ELF driver has no init function!: %p\xF7\n", init_passport);
        return 0;
    }
    struct device_driver* driver_struct = (struct device_driver*)kmalloc(sizeof(struct device_driver));
    if (driver_struct == 0) {
        print("\xFC""Insufficient memory to create driver info struct.\xF7\n");
        return 0;
    }

    driver_struct->driver_kind = DRIVER_KIND_GENERIC;
    driver_struct->init_function = init_function;
    driver_struct->init_result = init_function();
    driver_struct->driver_id = drivers_loaded_total;

    print("Querying driver classes/IDs...\n");
    uint32_t* suitable_vidpid = get_elf_symbol(init_passport, "__driver_vidpid");
    uint32_t* suitable_vidpid_count = get_elf_symbol(init_passport, "__driver_vidpid_count");

    if (suitable_vidpid == 0 || suitable_vidpid_count == 0) {
        driver_struct->driver_vidpid_count = 0;
        driver_struct->driver_vidpid = 0;
    } else {
        driver_struct->driver_vidpid_count = *suitable_vidpid_count;
        driver_struct->driver_vidpid = suitable_vidpid;
    }
    

    uint16_t* suitable_classes = get_elf_symbol(init_passport, "__driver_classes");
    uint32_t* suitable_classes_count = get_elf_symbol(init_passport, "__driver_classes_count");
    if (suitable_classes == 0 || suitable_classes_count == 0) {
        driver_struct->driver_classes_count = 0;
        driver_struct->driver_classes = 0;
    } else {
        driver_struct->driver_classes_count = *suitable_classes_count;
        driver_struct->driver_classes = suitable_classes;
    }

    uint16_t* driver_class_priority = get_elf_symbol(init_passport, "__driver_class_priority");
    driver_struct->driver_class_priority = driver_class_priority ? *driver_class_priority : 0;

    char* name_ptr = *(char**)get_elf_symbol(init_passport, "__driver_name");
    char* author_ptr = *(char**)get_elf_symbol(init_passport, "__driver_author");

    driver_struct->driver_name = name_ptr;
    driver_struct->driver_author = author_ptr;
    driver_struct->passport = init_passport;

    if (driver_tree_base) {
        driver_struct->next_drv = driver_tree_base;
    }
    driver_tree_base = driver_struct;
    
    
    drivers_loaded_total++;
    return driver_struct;
}

struct device_driver* register_driver(char* bin_driver, uint32_t bin_size) {
    //printf("\xFE""-- Loading driver %u from bin... --\n", drivers_loaded_total);
    struct driver_init_passport* driver_passport = load_driver_bin(bin_driver, bin_size);
    if (driver_passport == 0) {
        printf("\xFC""Failed to register driver at %p.\xF7\n", bin_driver);
        return 0;
    }
    return register_driver_i(driver_passport);
}

void device_from_pci(uint8_t bus, uint8_t device, uint8_t func) {
    uint16_t vendorID = pci_read_word(bus, device, func, 0);
    if (vendorID == 0xFFFF) return;

    uint16_t deviceID = pci_read_word(bus, device, func, 2);
    uint8_t deviceClass    = pci_read_byte(bus, device, func, 0x0B);
    uint8_t deviceSubclass = pci_read_byte(bus, device, func, 0x0A);
    if (func == 0) {
        uint8_t headerType = pci_read_word(bus, device, 0, 0xC) & 0xFF;
        if (headerType & 0x80) {
            for (uint8_t fn = 1; fn < 8; fn++) {
                device_from_pci(bus, device, fn);
            }
        }
    }
    printf("PCI b%u d%u f%u: %s: %s (%x/%x)\n", bus, device, func, pci_class_name(deviceClass), pci_subclass_name(deviceClass, deviceSubclass), vendorID, deviceID);
    struct device* dev_data = (struct device*)kmalloc(sizeof(struct device));
    if (dev_data == 0) {
        debugprintkmap();
        panic("Insufficient memory to create device info struct.\n");
    }
    dev_data->vendor_id = vendorID;
    dev_data->device_id = deviceID;
    dev_data->dev_function = func;
    dev_data->device_class = deviceClass;
    dev_data->device_subclass = deviceSubclass;
    uint16_t priority = 0xFFFF;
    dev_data->assigned_driver = find_driver(deviceClass, deviceSubclass, vendorID, deviceID, priority);
    while (dev_data->assigned_driver) {
        printf("\xFA" "Loaded driver: %s %s\xF7\n", dev_data->assigned_driver->driver_author, dev_data->assigned_driver->driver_name);
        driver_bind_pci driver_bind_func = get_elf_symbol(dev_data->assigned_driver->passport, "bind_device_pci");
        if (driver_bind_func) {
            int result = driver_bind_func(bus, device, func);
            if (result == 0) break;
            printf("\xFCThe driver has failed to load.\xF7\n");
            priority = dev_data->assigned_driver->driver_class_priority;
            if (priority == 0) break;
            dev_data->assigned_driver = find_driver(deviceClass, deviceSubclass, vendorID, deviceID, priority);
        }
    }
    if (!dev_data->assigned_driver) {
        printf("\xFCNo driver found for this device.\xF7\n");
    }
    
    if (device_tree_base) dev_data->next_dev = device_tree_base;
    device_tree_base = dev_data;
}

void init_devices() {
    uint16_t bus;
    uint8_t device;

    for (bus = 0; bus < 256; bus++) {
        for (device = 0; device < 32; device++)
        {
            device_from_pci(bus, device, 0);
        }
    }
}
