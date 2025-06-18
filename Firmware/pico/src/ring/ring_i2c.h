#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdatomic.h>
#include "gamepad/gamepad.h"
#include "ring/ring_shared.h"

typedef enum {
    RING_I2C_TYPE_NONE = 0,
    RING_I2C_TYPE_CONNECT,
    RING_I2C_TYPE_PAD,
    RING_I2C_TYPE_RUMBLE,
    RING_I2C_TYPE_PCM,
} ring_i2c_type_t;

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t type;
    uint8_t index;
    uint8_t reserved;
    uint8_t payload_len;
    uint8_t payload[];
} ring_i2c_packet_t;

#define RING_I2C_SIZE           (8U * GAMEPADS_MAX)
#define RING_I2C_BUF_SIZE       (sizeof(gamepad_pad_t) + sizeof(ring_i2c_packet_t))
#define RING_I2C_PAYLOAD_MAX    (RING_I2C_BUF_SIZE - sizeof(ring_i2c_packet_t))

typedef struct {
    uint8_t buf[RING_I2C_SIZE][RING_I2C_BUF_SIZE] __attribute__((aligned(4)));
    atomic_uint head;
    atomic_uint tail;
} ring_i2c_t;

static inline void ring_i2c_push(ring_i2c_t* ring, const ring_i2c_packet_t* header, const uint8_t* payload) {
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t next_head = (head + 1) % RING_I2C_SIZE;
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (next_head == tail) {
        atomic_store_explicit(&ring->tail, (tail + 1) % RING_I2C_SIZE, memory_order_release);
    }
    memcpy(ring->buf[head], header, sizeof(ring_i2c_packet_t));
    if (payload != NULL) {
        memcpy(&ring->buf[head][sizeof(ring_i2c_packet_t)], payload, 
               MIN(header->payload_len, RING_I2C_PAYLOAD_MAX));
    }
    atomic_store_explicit(&ring->head, next_head, memory_order_release);
}

static inline bool ring_i2c_pop(ring_i2c_t* ring, uint8_t* data) {
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    if (tail == head) {
        return false;
    }
    ring_i2c_packet_t* packet = (ring_i2c_packet_t*)ring->buf[tail];
    memcpy(data, ring->buf[tail], 
           MIN(sizeof(ring_i2c_packet_t) + packet->payload_len, RING_I2C_BUF_SIZE));
    atomic_store_explicit(&ring->tail, (tail + 1) % RING_I2C_SIZE, memory_order_release);
    return true;
}