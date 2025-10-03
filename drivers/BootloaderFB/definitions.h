#include <stdint.h>

typedef struct {
    uint32_t *addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
} framebuffer_t;

struct kfbmode {
    uint16_t width;
    uint16_t height;
} __attribute__((packed));