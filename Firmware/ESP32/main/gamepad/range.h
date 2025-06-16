#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MIN
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#define R_INT16_MAX   ((int16_t)32767)
#define R_INT16_MID   ((int16_t)0)
#define R_INT16_MIN   ((int16_t)-32768)

#define R_UINT8_MAX   ((uint8_t)255)
#define R_UINT8_MID   ((uint8_t)128)
#define R_UINT8_MIN   ((uint8_t)0)

#define R_UINT16_MAX  ((uint16_t)65535)
#define R_UINT16_MID  ((uint16_t)32768)
#define R_UINT16_MIN  ((uint16_t)0)

#define R_UINT10_MAX  ((uint16_t)1023)
#define R_UINT10_MID  ((uint16_t)512)
#define R_UINT10_MIN  ((uint16_t)0)

#define R_INT10_MAX   ((int16_t)511)
#define R_INT10_MID   ((int16_t)0)
#define R_INT10_MIN   ((int16_t)-512)

static inline int16_t range_uint8_to_int16(uint8_t value) {
    return (int16_t)(((int32_t)value << 8) - R_UINT16_MID);
}

static inline uint16_t range_uint8_to_uint16(uint8_t value) {
    return (uint16_t)value << 8;
}

static inline uint8_t range_int16_to_uint8(int16_t value) {
    return (uint8_t)(((int32_t)value + R_UINT16_MID) >> 8);
}

static inline uint8_t range_uint16_to_uint8(uint16_t value) {
    return (uint8_t)(value >> 8);
}

static inline int16_t range_uint10_to_int16(uint16_t value) {
    return (int16_t)(((int32_t)MIN(value, R_UINT10_MAX) << 6) - R_UINT16_MID);
}

static inline uint8_t range_uint10_to_uint8(uint16_t value) {
    return (uint8_t)(MIN(value, R_UINT10_MAX) >> 2);
}

static inline int16_t range_int10_to_int16(int16_t value) {
    if (value > R_INT10_MAX) {
        value = R_INT10_MAX;
    } else if (value < R_INT10_MIN) {
        value = R_INT10_MIN;
    }
    return value << 6;
}

static inline int16_t range_invert_int16(int16_t value) {
    return (int16_t)(-((int32_t)value) - 1);
}

static inline int16_t range_free_scale_int16(int32_t value, int32_t min, int32_t max) {
    if (max <= min) {
        return 0;
    } else if (value < min) {
        return R_INT16_MIN;
    } else if (value > max) {
        return R_INT16_MAX;
    }
    return (int16_t)(((value - min) * (int64_t)(R_INT16_MAX - R_INT16_MIN)) / (max - min) + R_INT16_MIN);
}

static inline uint8_t range_free_scale_uint8(int32_t value, int32_t min, int32_t max) {
    if (max <= min) {
        return 0;
    } else if (value < min) {
        return R_UINT8_MIN;
    } else if (value > max) {
        return R_UINT8_MAX;
    }
    return (uint8_t)(((value - min) * (int64_t)(R_UINT8_MAX - R_UINT8_MIN)) / (max - min) + R_UINT8_MIN);
}

#ifdef __cplusplus
}
#endif