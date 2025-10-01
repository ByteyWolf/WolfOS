#ifndef DRIVERMODEL_H
#define DRIVERMODEL_H

#include <stdint.h>
#include "../progman/elf.h"

enum {
    DRIVER_KIND_GENERIC = 0,
    DRIVER_KIND_FILESYSTEM = 1
};

struct device_driver {
    uint16_t driver_kind;
    driver_init_v1 init_function;
    int init_result;
    uint32_t driver_id;

    uint32_t driver_classes_count;
    uint32_t driver_vidpid_count;
    uint16_t* driver_classes;
    uint32_t* driver_vidpid;
    uint16_t driver_class_priority;

    char* driver_name;
    char* driver_author;

    struct driver_init_passport* passport;

    struct device_driver* next_drv;
};

struct device {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t dev_function;
    uint8_t device_class;
    uint8_t device_subclass;
    struct device_driver* assigned_driver;
    struct device* next_dev;
};

struct device_driver* register_driver(char* bin_driver, uint32_t bin_size);
struct device_driver* register_driver_i(struct driver_init_passport* init_passport);
void init_devices();

#endif /* DRIVERMODEL_H */
