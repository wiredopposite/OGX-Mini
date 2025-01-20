#include "UserSettings/TriggerSettings.h"

bool TriggerSettings::is_same(const TriggerSettingsRaw& raw) const
{
    return  dz_inner == Fix16(raw.dz_inner) &&
            dz_outer == Fix16(raw.dz_outer) &&
            anti_dz_inner == Fix16(raw.anti_dz_inner) &&
            anti_dz_outer == Fix16(raw.anti_dz_outer) &&
            curve == Fix16(raw.curve);
}

void TriggerSettings::set_from_raw(const TriggerSettingsRaw& raw)
{
    dz_inner = Fix16(raw.dz_inner);
    dz_outer = Fix16(raw.dz_outer);
    anti_dz_inner = Fix16(raw.anti_dz_inner);
    anti_dz_outer = Fix16(raw.anti_dz_outer);
    curve = Fix16(raw.curve);
}