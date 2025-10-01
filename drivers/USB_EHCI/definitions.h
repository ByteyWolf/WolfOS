#include "stdint.h"

#define LOG(msg) print("[EHCI] " msg "\n")
#define FAIL(msg) print("\xFC[EHCI] " msg "\xF7\n"); return 1

struct ehci_qh {
    uint32_t qhlp;
    uint32_t end_char;
    uint16_t end_cap;
    uint8_t uframe_c;
    uint8_t uframe_s;
    uint32_t current_qtd;
    uint32_t next_qtd;
    uint32_t alternate_nextqtd;
    uint32_t token;
    uint32_t bufptr[5];
    
} __attribute__((packed));

struct ehci_qtd {
    volatile uint32_t next_qtd;
    volatile uint32_t alt_next_qtd;
    volatile uint32_t token;
    volatile uint32_t buffer_ptr[5];
    volatile uint32_t reserved;
} __attribute__((packed));

struct usb_setup_packet {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed));

struct cap_regs {
    volatile uint8_t caplength;
    volatile uint8_t reserved;
    volatile uint16_t hciversion;
    volatile uint32_t hcsparams;
    volatile uint32_t hccparams;
    volatile uint32_t hcsp_portroute;
} __attribute__((packed));

struct op_regs {
    volatile uint32_t usbcmd;
    volatile uint32_t usbsts;
    volatile uint32_t usbintr;
    volatile uint32_t frindex;
    volatile uint32_t ctrldssegment;
    volatile uint32_t* periodiclistbase;
    volatile uint32_t asynclistaddr;
    volatile uint8_t reserved_1C[0x40 - 0x1C];
    volatile uint32_t configflag;
    volatile uint32_t *ports;
} __attribute__((packed));

#define USBLEGSUP 0x8E0
#define USBLEGSUP_HC_OS (1<<24)
#define USBLEGSUP_HC_BIOS (1<<16)

#define CONFIGFLAG 0x40
#define PORTSC 0x44

#define USBCMD_RUN 0
#define USBCMD_HCR 1
#define USBCMD_PFLS 2
#define USBCMD_PSE 4
#define USBCMD_ASE 5
#define USBCMD_IT 16

#define USBSTS_TRANSFER_I 0
#define USBSTS_ERROR_I 1
#define USBSTS_PORT_CHG 2
#define USBSTS_FRAME_LIST_ROLLOVER 3
#define USBSTS_HSE 4
#define USBSTS_DBI 5
#define USBSTS_HALTED 12
#define USBSTS_RECLAMATION 13
#define USBSTS_PERIOD_STATUS 14
#define USBSTS_ASYNC_STATUS 15

#define PORT_CONNECTED 0
#define PORT_CONNECTED_CHANGE 1
#define PORT_ENABLED 2
#define PORT_ENABLED_CHANGE 3
#define PORT_OVERCURRENT 4
#define PORT_FORCE_RESUME 6
#define PORT_RESET 8
#define PORT_LINESTATUS 10

#define QH_HLP 0x0
#define QH_ENDPOINT_CHAR 0x04
#define QH_ENDPOINT_CAP 0x08
#define QH_CURRENT_TD_A 0x0C
#define QH_CURRENT_TD_WA 0x10

#define HLP_TERMINATE 0
#define HLP_NEXT_TYPE 1
#define HLP_NEXT_QH 5

#define ENDCHAR_ADDRESS 0
#define ENDCHAR_INACTIVATE 7
#define ENDCHAR_ENDPOINT_NUM 8
#define ENDCHAR_SPEED 12
#define ENDCHAR_DATA_TOGGLE 14
#define ENDCHAR_HEAD_OF 15
#define ENDCHAR_MAX_LENGTH 16
#define ENDCHAR_CONTROL_ENDPOINT 27
#define ENDCHAR_NAK_RELOAD 28

#define ENDCAP_INTERRUPT_MASK 0
#define ENDCAP_SPLIT_COMPLETION_MASK 8
#define ENDCAP_HUB_ADDRESS 16
#define ENDCAP_PORT_NUMBER 23
#define ENDCAP_HBPM 30

extern void print(char* str);
extern void putchar(char chr);
extern uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
extern uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
extern uint8_t pci_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
extern void print_ptr(const void *p);
extern void* kmalloc(uint32_t nbytes);
extern void* kamalloc(uint32_t nbytes, uint32_t align);
extern uint8_t kfree(void* ptr);
extern void pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);
extern void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
extern void wait(uint32_t ms);


int get_bit(const unsigned char *data, uint32_t bit_offset);
void set_bit(unsigned char *data, uint32_t bit_offset, int value);

void print_u8(uint8_t n);
