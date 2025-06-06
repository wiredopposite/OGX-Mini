#include <string.h>
#include <stdatomic.h>
#include <pico/mutex.h>
#include "gamepad/gamepad.h"

#define GAMEPAD_FLAGS_IN_PAD_Msk         (GAMEPAD_FLAG_IN_PAD | GAMEPAD_FLAG_IN_PAD_ANALOG | GAMEPAD_FLAG_IN_CHATPAD)
#define GAMEPAD_FLAGS_IN_Msk             (GAMEPAD_FLAGS_IN_PAD_Msk | GAMEPAD_FLAG_IN_PCM)
#define GAMEPAD_FLAGS_OUT_RUMBLE_Msk     (GAMEPAD_FLAG_OUT_RUMBLE | GAMEPAD_FLAG_OUT_RUMBLE_DUR)
#define GAMEPAD_FLAGS_OUT_Msk            (GAMEPAD_FLAGS_OUT_RUMBLE_Msk | GAMEPAD_FLAG_OUT_PCM)

typedef struct __attribute__((aligned(64))) gamepad_handle_ {
    volatile bool       inited;
    atomic_bool         new_out;
    atomic_bool         new_in;
    volatile uint8_t    index;
    mutex_t             mutex;
    uint32_t            flags;
    gamepad_pad_t       pad;
    gamepad_rumble_t    rumble;
    gamepad_pcm_out_t   pcm_out;
    gamepad_pcm_in_t    pcm_in;
} gamepad_handle_t;

static gamepad_handle_t handles[GAMEPADS_MAX] = {0};
static size_t handles_size = sizeof(handles);

gamepad_handle_t* gamepad_init(uint8_t index) {
    if (index >= GAMEPADS_MAX) {
        return NULL;
    }
    gamepad_handle_t* handle = &handles[index];
    if (handle->inited) {
        return handle;
    }
    if (!mutex_is_initialized(&handle->mutex)) {
        memset(handle, 0, sizeof(gamepad_handle_t));
        mutex_init(&handle->mutex);
    }
    handle->inited = true;
    handle->index = index;
    return handle;
}

uint8_t gamepad_get_index(gamepad_handle_t* handle) {
    return handle->index;
}

/* ---- IN/Host Side methods ---- */

bool gamepad_new_out(gamepad_handle_t* handle) {
    return atomic_load_explicit(&handle->new_out, memory_order_acquire);
}

void gamepad_set_pad(gamepad_handle_t* handle, const gamepad_pad_t* pad, uint32_t flags) {
    mutex_enter_blocking(&handle->mutex);
    flags = ((flags & GAMEPAD_FLAGS_IN_PAD_Msk) | GAMEPAD_FLAG_IN_PAD);
    handle->flags |= flags;
    if ((flags & GAMEPAD_FLAG_IN_CHATPAD) == 0) {
        memcpy(&handle->pad, pad, offsetof(gamepad_pad_t, chatpad));
    } else {
        memcpy(&handle->pad, pad, sizeof(gamepad_pad_t));
    }
    atomic_store_explicit(&handle->new_in, true, memory_order_release);
    mutex_exit(&handle->mutex);
}

void gamepad_set_chatpad(gamepad_handle_t* handle, const uint8_t chatpad[3]) {
    mutex_enter_blocking(&handle->mutex);
    handle->flags |= GAMEPAD_FLAG_IN_CHATPAD;
    memcpy(handle->pad.chatpad, chatpad, sizeof(handle->pad.chatpad));
    atomic_store_explicit(&handle->new_in, true, memory_order_release);
    mutex_exit(&handle->mutex);
}

void gamepad_set_pcm_in(gamepad_handle_t* handle, const gamepad_pcm_in_t* pcm) {
    mutex_enter_blocking(&handle->mutex);
    handle->flags |= GAMEPAD_FLAG_IN_PCM;
    memcpy(&handle->pcm_in, pcm, sizeof(gamepad_pcm_in_t));
    atomic_store_explicit(&handle->new_in, true, memory_order_release);
    mutex_exit(&handle->mutex);
}

uint32_t gamepad_get_rumble(gamepad_handle_t* handle, gamepad_rumble_t* rumble_out) {
    mutex_enter_blocking(&handle->mutex);
    uint32_t flags = handle->flags & GAMEPAD_FLAGS_OUT_Msk;
    memcpy(rumble_out, &handle->rumble, sizeof(gamepad_rumble_t));
    handle->flags &= ~GAMEPAD_FLAGS_OUT_RUMBLE_Msk;
    if ((handle->flags & GAMEPAD_FLAGS_OUT_Msk) == 0) {
        atomic_store_explicit(&handle->new_out, false, memory_order_release);
    }
    mutex_exit(&handle->mutex);
    return flags;
}

uint32_t gamepad_get_pcm_out(gamepad_handle_t* handle, gamepad_pcm_out_t* pcm) {
    mutex_enter_blocking(&handle->mutex);
    uint32_t flags = handle->flags & GAMEPAD_FLAGS_OUT_Msk;
    memcpy(pcm, &handle->pcm_out, sizeof(gamepad_pcm_out_t));
    handle->flags &= ~GAMEPAD_FLAG_OUT_PCM;
    if ((handle->flags & GAMEPAD_FLAGS_OUT_Msk) == 0) {
        atomic_store_explicit(&handle->new_out, false, memory_order_release);
    }
    mutex_exit(&handle->mutex);
    return flags;
}

/* ---- OUT/Device side methods ---- */

bool gamepad_new_in(gamepad_handle_t* handle) {
    return atomic_load_explicit(&handle->new_in, memory_order_acquire);
}

void gamepad_set_rumble(gamepad_handle_t* handle, const gamepad_rumble_t* rumble) {
    mutex_enter_blocking(&handle->mutex);
    handle->flags |=    (GAMEPAD_FLAG_OUT_RUMBLE | 
                        (rumble->l_duration || rumble->r_duration) 
                            ? GAMEPAD_FLAG_OUT_RUMBLE_DUR : 0);
    memcpy(&handle->rumble, rumble, sizeof(gamepad_rumble_t));
    atomic_store_explicit(&handle->new_out, true, memory_order_release);
    mutex_exit(&handle->mutex);
}

void gamepad_set_pcm_out(gamepad_handle_t* handle, const gamepad_pcm_out_t* pcm) {
    mutex_enter_blocking(&handle->mutex);
    handle->flags |= GAMEPAD_FLAG_OUT_PCM;
    memcpy(&handle->pcm_out, pcm, sizeof(gamepad_pcm_out_t));
    atomic_store_explicit(&handle->new_out, true, memory_order_release);
    mutex_exit(&handle->mutex);
}

uint32_t gamepad_get_pad(gamepad_handle_t* handle, gamepad_pad_t* pad_out) {
    mutex_enter_blocking(&handle->mutex);
    memcpy(pad_out, &handle->pad, sizeof(gamepad_pad_t));
    uint32_t flags = handle->flags & GAMEPAD_FLAGS_IN_Msk;
    handle->flags &= ~GAMEPAD_FLAGS_IN_PAD_Msk;
    if ((handle->flags & GAMEPAD_FLAGS_IN_Msk) == 0) {
        atomic_store_explicit(&handle->new_in, false, memory_order_release);
    }
    mutex_exit(&handle->mutex);
    return flags;
}

uint32_t gamepad_get_pcm_in(gamepad_handle_t* handle, gamepad_pcm_in_t* pcm) {
    mutex_enter_blocking(&handle->mutex);
    uint32_t flags = handle->flags & GAMEPAD_FLAGS_IN_Msk;
    memcpy(pcm, &handle->pcm_in, sizeof(gamepad_pcm_in_t));
    handle->flags &= ~GAMEPAD_FLAG_IN_PCM;
    if ((handle->flags & GAMEPAD_FLAGS_IN_Msk) == 0) {
        atomic_store_explicit(&handle->new_in, false, memory_order_release);
    }
    mutex_exit(&handle->mutex);
    return flags;
}

// uint32_t gamepad_get_flags(gamepad_handle_t* handle) {
//     mutex_enter_blocking(&handle->mutex);
//     uint32_t flags = handle->flags;
//     mutex_exit(&handle->mutex);
//     return flags;
// }