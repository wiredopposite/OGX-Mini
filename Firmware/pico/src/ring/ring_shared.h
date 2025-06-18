#pragma once

#ifdef __cplusplus
#include <atomic>

typedef std::atomic<uint32_t> atomic_uint;
#define atomic_load_explicit(ptr, order)   (ptr)->load(order)
#define atomic_store_explicit(ptr, val, order) (ptr)->store(val, order)
#define memory_order_relaxed   std::memory_order_relaxed
#define memory_order_acquire   std::memory_order_acquire
#define memory_order_release   std::memory_order_release

#else
#include <stdatomic.h>
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif