#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdatomic.h>
#include "board_config.h"
#include "gamepad/gamepad.h"

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define WIRED_RING_SIZE (8U * GAMEPADS_MAX)

#if (BLUETOOTH_HARDWARE == BLUETOOTH_HARDWARE_ESP32_I2C)
#define WIRED_MAX_SIZE (sizeof(gamepad_pad_t) + sizeof(wired_packet_t))
#else
#define WIRED_MAX_SIZE (MAX(sizeof(gamepad_pad_t), sizeof(gamepad_pcm_in_t)) + sizeof(wired_packet_t))
#endif

typedef enum {
    WIRED_REPORT_ID_NONE = 0,
    WIRED_REPORT_ID_CONNECT,
    WIRED_REPORT_ID_PAD,
    WIRED_REPORT_ID_RUMBLE,
    WIRED_REPORT_ID_PCM,
} wired_report_id_t;

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t report_id;
    uint8_t index;
    uint8_t reserved;
    uint8_t payload_len;
    uint8_t payload[];
} wired_packet_t;
_Static_assert(sizeof(wired_packet_t) == 4, "Wired packet size mismatch");

typedef struct {
    uint8_t buf[WIRED_RING_SIZE][WIRED_MAX_SIZE] __attribute__((aligned(4)));
    atomic_uint head;
    atomic_uint tail;
} ring_wired_t;

static inline bool ring_wired_push(ring_wired_t* ring, const uint8_t* data) {
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t next_head = (head + 1) % WIRED_RING_SIZE;
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (next_head == tail) {
        atomic_store_explicit(&ring->tail, (tail + 1) % WIRED_RING_SIZE, memory_order_release);
    }
    memcpy(ring->buf[head], data, WIRED_MAX_SIZE);
    atomic_store_explicit(&ring->head, next_head, memory_order_release);
    return true;
}

static inline bool ring_wired_pop(ring_wired_t* ring, uint8_t* data) {
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    if (tail == head) {
        return false;
    }
    memcpy(data, ring->buf[tail], WIRED_MAX_SIZE);
    atomic_store_explicit(&ring->tail, (tail + 1) % WIRED_RING_SIZE, memory_order_release);
    return true;
}