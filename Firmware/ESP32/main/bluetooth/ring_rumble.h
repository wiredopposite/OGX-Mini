#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdatomic.h>
#include "gamepad/gamepad.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RUBMLE_RING_SIZE 8U

typedef struct {
    gamepad_rumble_t buf[RUBMLE_RING_SIZE] __attribute__((aligned(4)));
    uint8_t index[RUBMLE_RING_SIZE];
    atomic_uint head;
    atomic_uint tail;
} ring_rumble_t;

static inline bool ring_rumble_push(ring_rumble_t* ring, uint8_t index, const gamepad_rumble_t* rumble) {
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t next_head = (head + 1) % RUBMLE_RING_SIZE;
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (next_head == tail) {
        atomic_store_explicit(&ring->tail, (tail + 1) % RUBMLE_RING_SIZE, memory_order_release);
    }
    ring->index[head] = index;
    memcpy(&ring->buf[head], rumble, sizeof(gamepad_rumble_t));
    atomic_store_explicit(&ring->head, next_head, memory_order_release);
    return true;
}

static inline bool ring_rumble_pop(ring_rumble_t* ring, uint8_t* index, gamepad_rumble_t* rumble) {
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    if (tail == head) {
        return false;
    }
    *index = ring->index[tail];
    memcpy(rumble, &ring->buf[tail], sizeof(gamepad_rumble_t));
    atomic_store_explicit(&ring->tail, (tail + 1) % RUBMLE_RING_SIZE, memory_order_release);
    return true;
}

static inline bool ring_rumble_empty(const ring_rumble_t* ring) {
    return (atomic_load_explicit(&ring->head, memory_order_relaxed) ==
            atomic_load_explicit(&ring->tail, memory_order_acquire));
}

#ifdef __cplusplus
}
#endif