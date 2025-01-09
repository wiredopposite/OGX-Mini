#ifndef _OGX_MINI_H_
#define _OGX_MINI_H_

#include <cstdint>

#include "UserSettings/UserSettings.h"

namespace OGXMini
{
    static constexpr int32_t FEEDBACK_DELAY_MS = 150;
    static constexpr int32_t TUD_CHECK_DELAY_MS = 500;

    void run_program();
    void host_mounted(bool mounted);

} // namespace OGXMini

#endif // _OGX_MINI_H_