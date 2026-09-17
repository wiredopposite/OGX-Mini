/* Maple Bus timing and pin config. Adapted from DreamPicoPort configuration.h */

#pragma once

#ifndef MAPLE_OPEN_LINE_CHECK_TIME_US
#define MAPLE_OPEN_LINE_CHECK_TIME_US 10
#endif

#ifndef MAPLE_NS_PER_BIT
#define MAPLE_NS_PER_BIT 480
#endif

#ifndef MAPLE_WRITE_TIMEOUT_EXTRA_PERCENT
#define MAPLE_WRITE_TIMEOUT_EXTRA_PERCENT 20
#endif

#ifndef MAPLE_RESPONSE_TIMEOUT_US
#define MAPLE_RESPONSE_TIMEOUT_US 1000
#endif

#ifndef MAPLE_INTER_WORD_READ_TIMEOUT_US
#define MAPLE_INTER_WORD_READ_TIMEOUT_US 300
#endif

/* CPU frequency in kHz (RP2040 default 125000) */
#ifndef MAPLE_CPU_FREQ_KHZ
#define MAPLE_CPU_FREQ_KHZ 125000
#endif
