#pragma once 

// #if !defined(__cplusplus) && (__STDC_VERSION__ < 201112L)
// # ifndef static_assert
// #  define static_assert(cond, msg) typedef char static_assertion_##__LINE__[(cond)?1:-1]
// # endif
// #endif

// #ifdef __cplusplus
// #define _STATIC_ASSERT(cond, msg) static_assert(cond, msg)
// #else
#define _STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
// #endif