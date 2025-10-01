#include <cstdint>
#include <stdint.h>

struct __attribute__((packed)) framebuf {
    void* fbase;
    uint32_t width;
    uint32_t height;
    uint8_t planes;
    uint8_t plane_mode;
};

const uint8_t PLANE_MODE_SWAPPABLE = 0; // must swap plane on our own

