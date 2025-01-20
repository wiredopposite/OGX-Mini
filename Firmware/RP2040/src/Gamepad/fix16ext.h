#ifndef FIX16_EXT_H
#define FIX16_EXT_H

#include <cstdint>

#include "libfixmath/fix16.hpp"

namespace fix16 {

inline Fix16 abs(Fix16 x)
{
    return Fix16(fix16_abs(x.value));
}

inline Fix16 rad2deg(Fix16 rad)
{
    return Fix16(fix16_rad_to_deg(rad.value));
}

inline Fix16 deg2rad(Fix16 deg)
{
    return Fix16(fix16_deg_to_rad(deg.value));
}

inline Fix16 atan(Fix16 x)
{
    return Fix16(fix16_atan(x.value));
}

inline Fix16 atan2(Fix16 y, Fix16 x)
{
    return Fix16(fix16_atan2(y.value, x.value));
}

inline Fix16 tan(Fix16 x)
{
    return Fix16(fix16_tan(x.value));
}

inline Fix16 cos(Fix16 x)
{
    return Fix16(fix16_cos(x.value));
}

inline Fix16 sin(Fix16 x)
{
    return Fix16(fix16_sin(x.value));
}

inline Fix16 sqrt(Fix16 x)
{
    return Fix16(fix16_sqrt(x.value));
}

inline Fix16 sq(Fix16 x)
{
    return Fix16(fix16_sq(x.value));
}

inline Fix16 clamp(Fix16 x, Fix16 min, Fix16 max)
{
    return Fix16(fix16_clamp(x.value, min.value, max.value));
}

inline Fix16 pow(Fix16 x, Fix16 y)
{
    fix16_t& base = x.value;
    fix16_t& exponent = y.value;

    if (exponent == F16(0.0)) 
        return Fix16(fix16_from_int(1));
    if (base == F16(0.0)) 
        return Fix16(fix16_from_int(0));

    int32_t int_exp = fix16_to_int(exponent);

    if (fix16_from_int(int_exp) == exponent) 
    {
        fix16_t result = F16(1.0);
        fix16_t current_base = base;

        if (int_exp < 0) 
        {
            current_base = fix16_div(F16(1.0), base);
            int_exp = -int_exp;
        }

        while (int_exp) 
        {
            if (int_exp & 1) 
            {
                result = fix16_mul(result, current_base);
            }
            current_base = fix16_mul(current_base, current_base);
            int_exp >>= 1;
        }

        return Fix16(result);
    }

    return Fix16(fix16_exp(fix16_mul(exponent, fix16_log(base))));
}

} // namespace fix16

#endif // FIX16_EXT_H