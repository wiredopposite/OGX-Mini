#ifndef TRIGGER_SETTINGS_H
#define TRIGGER_SETTINGS_H

#include <cstdint>

#include "libfixmath/fix16.hpp"

struct TriggerSettingsRaw;

struct TriggerSettings
{
    Fix16 dz_inner{Fix16(0.0f)};
    Fix16 dz_outer{Fix16(1.0f)};

    Fix16 anti_dz_inner{Fix16(0.0f)};
    Fix16 anti_dz_outer{Fix16(1.0f)};

    Fix16 curve{Fix16(1.0f)};

    bool is_same(const TriggerSettingsRaw& raw) const;
    void set_from_raw(const TriggerSettingsRaw& raw);
};

#pragma pack(push, 1)
struct TriggerSettingsRaw
{
    fix16_t dz_inner{fix16_from_int(0)};
    fix16_t dz_outer{fix16_from_int(1)};

    fix16_t anti_dz_inner{fix16_from_int(0)};
    fix16_t anti_dz_outer{fix16_from_int(1)};
    
    fix16_t curve{fix16_from_int(1)};

    void log_values();
};
static_assert(sizeof(TriggerSettingsRaw) == 20, "TriggerSettingsRaw is an unexpected size");
#pragma pack(pop)

#endif // TRIGGER_SETTINGS_H