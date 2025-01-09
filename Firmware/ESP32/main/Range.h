#ifndef _RANGE_H_
#define _RANGE_H_

#include <cstdint>
#include <limits>
#include <type_traits>
#include "Board/ogxm_log.h"

namespace Range {

    template <typename T>
    requires std::is_integral_v<T>
    constexpr T MAX = std::numeric_limits<T>::max();

    template <typename T>
    requires std::is_integral_v<T>
    constexpr T MIN = std::numeric_limits<T>::min();

    template <typename T>
    requires std::is_integral_v<T>
    constexpr T MID = 
        [] {
            if constexpr (std::is_unsigned_v<T>) 
            {
                return MAX<T> / 2 + 1;
            } 
            return 0;
        }();
    static_assert(MID<uint8_t> == 128, "MID<uint8_t> != 128");
    static_assert(MID<int8_t> == 0, "MID<int8_t> != 0");

    template <typename T>
    requires std::is_integral_v<T>
    constexpr uint16_t NUM_BITS = sizeof(T) * 8;

    //Maximum value for a given number of bits, result will be signed/unsigned depending on type
    template<typename T, uint8_t bits>
    requires std::is_integral_v<T>
    constexpr T BITS_MAX() 
    {
        static_assert(bits <= NUM_BITS<T>, "BITS_MAX: Return type cannot represet maximum bit value");
        static_assert(bits <= 64, "BITS_MAX: Bits exceed 64");
        if constexpr (std::is_unsigned_v<T>) 
        {
            return (1ULL << bits) - 1;
        }
        return (1LL << (bits - 1)) - 1;
    }
    static_assert(BITS_MAX<int16_t, 10>() == 511, "BITS_MAX<int16_t>(10) != 511");
    static_assert(BITS_MAX<uint16_t, 10>() == 1023, "BITS_MAX<uint16_t>(10) != 1023");

    //Minimum value for a given number of bits, result will be signed/unsigned depending on type
    template<typename T, uint8_t bits>
    requires std::is_integral_v<T>
    constexpr T BITS_MIN() 
    {
        static_assert(bits <= NUM_BITS<T>, "BITS_MIN: Return type cannot represet minimum bit value");
        static_assert(bits <= 64, "BITS_MIN: Bits exceed 64");
        if constexpr (std::is_unsigned_v<T>) 
        {
            return 0;
        }
        return static_cast<T>(-(1LL << (bits - 1)));
    }
    static_assert(BITS_MIN<int16_t, 10>() == -512, "BITS_MIN<int16_t>(10) != -512");
    static_assert(BITS_MIN<uint16_t, 10>() == 0, "BITS_MIN<uint16_t>(10) != 0");

    template <typename T>
    static inline T invert(T value) 
    {   
        if constexpr (std::is_unsigned_v<T>) 
        {
            return Range::MAX<T> - value;
        } 
        return (value == Range::MIN<T>) ? Range::MAX<T> : -value; 
    }

    template <typename To, typename From>
    requires std::is_integral_v<To> && std::is_integral_v<From>
    static inline To clamp(From value) 
    {
        if constexpr (std::is_signed_v<From> != std::is_signed_v<To>)
        {
            using CommonType = std::common_type_t<To, From>;
            return static_cast<To>((static_cast<CommonType>(value) < static_cast<CommonType>(Range::MIN<To>)) 
                                    ? Range::MIN<To> 
                                    : (static_cast<CommonType>(value) > static_cast<CommonType>(Range::MAX<To>)) 
                                        ? Range::MAX<To> 
                                        : static_cast<CommonType>(value));
        }
        else
        {
            return static_cast<To>((value < Range::MIN<To>) 
                                    ? Range::MIN<To> 
                                    : (value > Range::MAX<To>) 
                                        ? Range::MAX<To> 
                                        : value);
        }
    }

    template <typename T>
    requires std::is_integral_v<T>
    static inline T clamp(T value, T min, T max) 
    {
        return (value < min) ? min : (value > max) ? max : value;
    }

    template <typename To, typename From>
    requires std::is_integral_v<To> && std::is_integral_v<From>
    static inline To scale(From value, From min_from, From max_from, To min_to, To max_to) 
    {
        if constexpr (std::is_unsigned_v<From> && std::is_unsigned_v<To>) 
        {
            // Both unsigned
            uint64_t scaled = static_cast<uint64_t>(value - min_from) * 
                            (max_to - min_to) / 
                            (max_from - min_from) + min_to;
            return static_cast<To>(scaled);
        } 
        else if constexpr (std::is_signed_v<From> && std::is_unsigned_v<To>) 
        {
            // From signed, To unsigned
            uint64_t shift_from = static_cast<uint64_t>(-min_from);
            uint64_t u_value = static_cast<uint64_t>(value) + shift_from;
            uint64_t u_min_from = static_cast<uint64_t>(min_from) + shift_from;
            uint64_t u_max_from = static_cast<uint64_t>(max_from) + shift_from;

            uint64_t scaled = (u_value - u_min_from) * 
                            (max_to - min_to) / 
                            (u_max_from - u_min_from) + min_to;
            return static_cast<To>(scaled);
        } 
        else if constexpr (std::is_unsigned_v<From> && std::is_signed_v<To>) 
        {
            // From unsigned, To signed
            uint64_t shift_to = static_cast<uint64_t>(-min_to);
            uint64_t scaled = static_cast<uint64_t>(value - min_from) * 
                            (static_cast<uint64_t>(max_to) + shift_to - static_cast<uint64_t>(min_to) - shift_to) / 
                            (max_from - min_from) + static_cast<uint64_t>(min_to) + shift_to;
            return static_cast<To>(scaled - shift_to);
        } 
        else 
        {
            // Both signed
            int64_t shift_from = -min_from;
            int64_t shift_to = -min_to;

            int64_t scaled = (static_cast<int64_t>(value) + shift_from - (min_from + shift_from)) * 
                            (max_to + shift_to - (min_to + shift_to)) / 
                            (max_from - min_from) + (min_to + shift_to);
            return static_cast<To>(scaled - shift_to);
        }
    }

    template <typename To, typename From>
    static inline To scale(From value) 
    {
        return scale<To>(value, Range::MIN<From>, Range::MAX<From>, Range::MIN<To>, Range::MAX<To>);
    }

    template <typename To, typename From>
    static inline To scale(From value, To min_to, To max_to) 
    {
        return scale<To>(value, Range::MIN<From>, Range::MAX<From>, min_to, max_to);
    }

    //Cast value to signed/unsigned for accurate scaling
    template <typename To, uint8_t bits, typename From>
    static inline To scale_from_bits(From value) 
    {
        return scale<To>(
            value, 
            BITS_MIN<From, bits>(), 
            BITS_MAX<From, bits>(), 
            Range::MIN<To>, 
            Range::MAX<To>);
    }

    //Cast value to signed/unsigned for accurate scaling
    template <typename To, uint8_t bits, typename From>
    static inline To scale_to_bits(From value) 
    {
        return scale<To>(
            value, 
            Range::MIN<From>, 
            Range::MAX<From>, 
            BITS_MIN<To, bits>(), 
            BITS_MAX<To, bits>());
    }

} // namespace Range

namespace Scale //Scale and invert values
{
    static inline uint8_t int16_to_uint8(int16_t value)
    {
        uint16_t shifted_value = static_cast<uint16_t>(value + Range::MID<uint16_t>);
        return static_cast<uint8_t>(shifted_value >> 8);
    }
    static inline uint16_t int16_to_uint16(int16_t value)
    {
        return static_cast<uint16_t>(value + Range::MID<uint16_t>);
    }
    static inline int8_t int16_to_int8(int16_t value)
    {
        return static_cast<int8_t>((value + Range::MID<uint16_t>) >> 8);
    }

    static inline uint8_t uint16_to_uint8(uint16_t value)
    {
        return static_cast<uint8_t>(value >> 8);
    }
    static inline int16_t uint16_to_int16(uint16_t value)
    {
        return static_cast<int16_t>(value - Range::MID<uint16_t>);
    }
    static inline int8_t uint16_to_int8(uint16_t value)
    {
        return static_cast<int8_t>((value >> 8) - Range::MID<uint8_t>);
    }

    static inline int16_t uint8_to_int16(uint8_t value)
    {
        return static_cast<int16_t>((static_cast<int32_t>(value) << 8) - Range::MID<uint16_t>);
    }
    static inline uint16_t uint8_to_uint16(uint8_t value)
    {
        return static_cast<uint16_t>(value) << 8;
    }
    static inline int8_t uint8_to_int8(uint8_t value)
    {
        return static_cast<int8_t>(value - Range::MID<uint8_t>);
    }

    static inline int16_t int8_to_int16(int8_t value)
    {
        return static_cast<int16_t>(value) << 8;
    }
    static inline uint16_t int8_to_uint16(int8_t value)
    {
        return static_cast<uint16_t>((value + Range::MID<uint8_t>) << 8);
    }
    static inline uint8_t int8_to_uint8(int8_t value)
    {
        return static_cast<uint8_t>(value + Range::MID<uint8_t>);
    }

} // namespace Scale

#endif // _RANGE_H_