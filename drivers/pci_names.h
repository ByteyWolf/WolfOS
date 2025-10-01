#ifndef PCI_NAMES_H
#define PCI_NAMES_H

#include <stdint.h>

const char *pci_class_name(uint8_t class_code);
const char *pci_subclass_name(uint8_t class_code, uint8_t subclass_code);

#endif /* PCI_NAMES_H */
