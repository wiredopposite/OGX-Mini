#ifndef OGXM_WII_UTILS_H
#define OGXM_WII_UTILS_H

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Save Wii console BD address to flash (for reconnection). Stub: no-op until we add persistence. */
void save_wii_addr(void *wii_addr);

#ifdef __cplusplus
}
#endif

#endif /* OGXM_WII_UTILS_H */
