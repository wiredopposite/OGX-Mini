#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "gamepad/gamepad.h"

#ifdef __cplusplus
#include <atomic>
typedef std::atomic<uint32_t> atomic_uint;
#define atomic_load_explicit(ptr, order)   (ptr)->load(order)
#define atomic_store_explicit(ptr, val, order) (ptr)->store(val, order)
#define memory_order_relaxed   std::memory_order_relaxed
#define memory_order_acquire   std::memory_order_acquire
#define memory_order_release   std::memory_order_release
#else
#include <stdatomic.h>
#endif

#define RING_USB_SIZE (8U * GAMEPADS_MAX)

typedef enum {
    RING_USB_TYPE_NONE = 0,
    RING_USB_TYPE_PAD,
    RING_USB_TYPE_RUMBLE,
    RING_USB_TYPE_PCM,
} ring_usb_type_t;

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t  type;
    uint8_t  index;
    uint8_t  reserved[2];
    uint32_t flags;
    uint8_t  payload[sizeof(gamepad_pcm_t)];
} ring_usb_buf_t;
_Static_assert(sizeof(ring_usb_buf_t) == 44, "Ring USB buffer size mismatch");

typedef struct {
    ring_usb_buf_t buf[RING_USB_SIZE];
    atomic_uint head;
    atomic_uint tail;
} ring_usb_t;

static inline bool ring_usb_push(ring_usb_t* ring, const ring_usb_buf_t* data) {
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t next_head = (head + 1) % RING_USB_SIZE;
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (next_head == tail) {
        atomic_store_explicit(&ring->tail, (tail + 1) % RING_USB_SIZE, memory_order_release);
    }
    memcpy(&ring->buf[head], data, sizeof(ring_usb_buf_t));
    atomic_store_explicit(&ring->head, next_head, memory_order_release);
    return true;
}

static inline bool ring_usb_pop(ring_usb_t* ring, ring_usb_buf_t* data) {
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);

    if (tail == head) {
        return false;
    }
    memcpy(data, &ring->buf[tail], sizeof(ring_usb_buf_t));
    atomic_store_explicit(&ring->tail, (tail + 1) % RING_USB_SIZE, memory_order_release);
    return true;
}
