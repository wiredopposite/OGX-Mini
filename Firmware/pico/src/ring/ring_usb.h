#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdatomic.h>
#include "gamepad/gamepad.h"
#include "ring/ring_shared.h"

typedef enum {
    RING_USB_TYPE_NONE = 0,
    RING_USB_TYPE_PAD,
    RING_USB_TYPE_RUMBLE,
    RING_USB_TYPE_PCM,
} ring_usb_type_t;

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t type;
    uint8_t index;
    uint8_t reserved;
    uint8_t payload_len;
    uint8_t payload[];
} ring_usb_packet_t;

#define RING_USB_SIZE           (8U * GAMEPADS_MAX)
#define RING_USB_BUF_SIZE       (MAX(sizeof(gamepad_pad_t), sizeof(gamepad_pcm_t)) + sizeof(ring_usb_packet_t))
#define RING_USB_PAYLOAD_MAX    (RING_USB_BUF_SIZE - sizeof(ring_usb_packet_t))

typedef struct {
    uint8_t buf[RING_USB_SIZE][RING_USB_BUF_SIZE] __attribute__((aligned(4)));
    atomic_uint head;
    atomic_uint tail;
} ring_usb_t;

static inline void ring_usb_push(ring_usb_t* ring, const ring_usb_packet_t* header, const void* payload) {
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t next_head = (head + 1) % RING_USB_SIZE;
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (next_head == tail) {
        atomic_store_explicit(&ring->tail, (tail + 1) % RING_USB_SIZE, memory_order_release);
    }
    memcpy(ring->buf[head], header, sizeof(ring_usb_packet_t));
    if (payload != NULL) {
        memcpy(&ring->buf[head][sizeof(ring_usb_packet_t)], payload, 
               MIN(header->payload_len, RING_USB_PAYLOAD_MAX));
    }
    atomic_store_explicit(&ring->head, next_head, memory_order_release);
}

static inline bool ring_usb_pop(ring_usb_t* ring, void* data) {
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    if (tail == head) {
        return false;
    }
    ring_usb_packet_t* packet = (ring_usb_packet_t*)ring->buf[tail];
    memcpy(data, ring->buf[tail], 
           MIN(sizeof(ring_usb_packet_t) + packet->payload_len, RING_USB_BUF_SIZE));
    atomic_store_explicit(&ring->tail, (tail + 1) % RING_USB_SIZE, memory_order_release);
    return true;
}

static inline bool ring_usb_empty(const ring_usb_t* ring) {
    return (atomic_load_explicit(&ring->head, memory_order_relaxed) == 
           atomic_load_explicit(&ring->tail, memory_order_acquire));
}