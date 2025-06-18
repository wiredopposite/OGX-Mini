#pragma once

#include "board_config.h"
#if (OGXM_BOARD == OGXM_BOARD_FOUR_CHANNEL)

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdatomic.h>
#include "gamepad/gamepad.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FOUR_CH_RING_SIZE (8U * GAMEPADS_MAX)
#define FOUR_CH_MAX_SIZE (sizeof(gamepad_pad_t) + sizeof(four_ch_packet_t))

typedef enum {
    FOURCH_REPORT_ID_NONE = 0,
    FOURCH_REPORT_ID_ENABLED,
    FOURCH_REPORT_ID_PAD,
    FOURCH_REPORT_ID_RUMBLE,
    FOURCH_REPORT_ID_PCM,
} wired_report_id_t;

typedef struct __attribute__((packed, aligned(4))) {
    uint8_t report_id;
    uint8_t index;
    uint8_t flags;
    uint8_t payload_len;
    uint8_t payload[];
} four_ch_packet_t;
_Static_assert(sizeof(four_ch_packet_t) == 4, "four_ch_packet_t size mismatch");

typedef struct {
    uint8_t buf[FOUR_CH_RING_SIZE][FOUR_CH_MAX_SIZE] __attribute__((aligned(4)));
    atomic_uint head;
    atomic_uint tail;
} ring_four_ch_t;

static inline bool ring_four_ch_push(ring_four_ch_t* ring, const four_ch_packet_t* header, const void* data) {
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t next_head = (head + 1) % FOUR_CH_RING_SIZE;
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (next_head == tail) {
        atomic_store_explicit(&ring->tail, (tail + 1) % FOUR_CH_RING_SIZE, memory_order_release);
    }
    memcpy(ring->buf[head], header, sizeof(four_ch_packet_t));
    if (data) {
        memcpy(ring->buf[head] + sizeof(four_ch_packet_t), data, header->payload_len);
    }
    atomic_store_explicit(&ring->head, next_head, memory_order_release);
    return true;
}

static inline bool ring_four_ch_pop(ring_four_ch_t* ring, uint8_t* data) {
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
    if (tail == head) {
        return false;
    }
    four_ch_packet_t* packet = (four_ch_packet_t*)ring->buf[tail];
    memcpy(data, ring->buf[tail], sizeof(four_ch_packet_t) + packet->payload_len);
    atomic_store_explicit(&ring->tail, (tail + 1) % FOUR_CH_RING_SIZE, memory_order_release);
    return true;
}

#ifdef __cplusplus
}
#endif

#endif // (OGXM_BOARD == OGXM_BOARD_FOUR_CHANNEL)