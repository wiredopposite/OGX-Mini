#include <stdint.h>
#include <stdbool.h>
#include <pico/sync.h>
#include <pico/time.h>
#include "lwip/sys.h"

static mutex_t* get_lwip_mutex(void) {
    static mutex_t mutex;
    if (!mutex_is_initialized(&mutex)) {
        mutex_init(&mutex);
    }
    return &mutex;
}

sys_prot_t sys_arch_protect(void) {
    mutex_enter_blocking(get_lwip_mutex());
    return 0;
}

void sys_arch_unprotect(sys_prot_t pval) {
    (void)pval;
    mutex_exit(get_lwip_mutex());
}

uint32_t sys_now(void) {
    return to_ms_since_boot(get_absolute_time());
}