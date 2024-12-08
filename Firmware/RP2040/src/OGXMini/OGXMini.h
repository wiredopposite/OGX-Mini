#ifndef _OGX_MINI_H_
#define _OGX_MINI_H_

#include <cstdint>

#include "UserSettings/UserSettings.h"

namespace OGXMini
{
    enum class TUDStatus { INIT, DEINIT, NOCHANGE };
    
    struct GPCheckContext
    {
        bool driver_changed;
        UserSettings& user_settings;
    };

    static constexpr int32_t FEEDBACK_DELAY_MS = 100;
    static constexpr int32_t TUD_CHECK_DELAY_MS = 500;

    void run_program();

    // Callback to notify main loop of tuh mounted
    void update_tuh_status(bool mounted) __attribute__((weak));
}

#endif // _OGX_MINI_H_