#ifndef _MOTION_IMU_H_
#define _MOTION_IMU_H_

#include <cstdint>
#include <cstring>

#include "Gamepad/Gamepad.h"

namespace MotionImu
{

constexpr int32_t kDs4AccRange = 4 * 8192;
constexpr int32_t kDs4GyroRange = 2048 * 1024;
constexpr int32_t kRawSensDenom = 32767;

constexpr int32_t kSwitchDefaultAccelScale = 16384;
constexpr int32_t kSwitchDefaultGyroScale = 13371;
constexpr int32_t kSwitchGyroPrecScale = 1000;
constexpr int32_t kWiiAccelCountsPerG = 0x6C;
constexpr int32_t kDs4UnitsPerG = 8192;

#pragma pack(push, 1)
struct SwitchImuSample
{
    int16_t accel[3];
    int16_t gyro[3];
};
#pragma pack(pop)

static_assert(sizeof(SwitchImuSample) == 12);

inline int32_t ds4_ds5_usb_raw_to_units(int16_t raw, int32_t range)
{
    return static_cast<int32_t>((static_cast<int64_t>(range) * static_cast<int64_t>(raw)) / kRawSensDenom);
}

inline void fill_from_ds4_usb_report(int32_t accel[3], int32_t gyro[3], const uint8_t* report)
{
    int16_t raw_g[3];
    int16_t raw_a[3];
    std::memcpy(&raw_g[0], &report[13], 2);
    std::memcpy(&raw_g[1], &report[15], 2);
    std::memcpy(&raw_g[2], &report[17], 2);
    std::memcpy(&raw_a[0], &report[19], 2);
    std::memcpy(&raw_a[1], &report[21], 2);
    std::memcpy(&raw_a[2], &report[23], 2);
    for (int i = 0; i < 3; i++) {
        gyro[i] = ds4_ds5_usb_raw_to_units(raw_g[i], kDs4GyroRange);
        accel[i] = ds4_ds5_usb_raw_to_units(raw_a[i], kDs4AccRange);
    }
}

inline void fill_from_ds5_usb_raw(int32_t accel[3], int32_t gyro[3], const uint16_t raw_accel[3], const uint16_t raw_gyro[3])
{
    for (int i = 0; i < 3; i++) {
        gyro[i] = ds4_ds5_usb_raw_to_units(static_cast<int16_t>(raw_gyro[i]), kDs4GyroRange);
        accel[i] = ds4_ds5_usb_raw_to_units(static_cast<int16_t>(raw_accel[i]), kDs4AccRange);
    }
}

/* Bluepad default factory cal when SPI calib is unavailable (wired USB). */
inline void fill_from_switch_usb_payload(int32_t accel[3], int32_t gyro[3], const uint8_t* payload, uint16_t payload_len)
{
    constexpr uint16_t kInReportSize = 10;
    constexpr uint16_t kImuSampleSize = sizeof(SwitchImuSample);

    if (payload_len < kInReportSize + kImuSampleSize) {
        for (int i = 0; i < 3; i++) {
            accel[i] = 0;
            gyro[i] = 0;
        }
        return;
    }

    uint16_t imu_off = kInReportSize;
    if (payload_len >= kInReportSize + (3 * kImuSampleSize)) {
        imu_off = static_cast<uint16_t>(kInReportSize + (2 * kImuSampleSize));
    }

    SwitchImuSample raw{};
    std::memcpy(&raw, payload + imu_off, sizeof(raw));

    const int32_t accel_div = kSwitchDefaultAccelScale;
    const int32_t gyro_div = kSwitchDefaultGyroScale;
    for (int i = 0; i < 3; i++) {
        if (accel_div != 0) {
            accel[i] = (static_cast<int32_t>(raw.accel[i]) * kSwitchDefaultAccelScale) / accel_div;
        } else {
            accel[i] = raw.accel[i];
        }
        if (gyro_div != 0) {
            const int64_t num = static_cast<int64_t>(kSwitchGyroPrecScale) * static_cast<int32_t>(raw.gyro[i]);
            gyro[i] = static_cast<int32_t>((num * kSwitchDefaultGyroScale) / gyro_div);
        } else {
            gyro[i] = raw.gyro[i];
        }
    }
}

inline bool switch_usb_imu_unchanged(const uint8_t* prev_imu, const uint8_t* payload, uint16_t payload_len)
{
    constexpr uint16_t kInReportSize = 10;
    constexpr uint16_t kImuSampleSize = sizeof(SwitchImuSample);
    if (payload_len < kInReportSize + kImuSampleSize) {
        return true;
    }

    uint16_t imu_off = kInReportSize;
    if (payload_len >= kInReportSize + (3 * kImuSampleSize)) {
        imu_off = static_cast<uint16_t>(kInReportSize + (2 * kImuSampleSize));
    }

    return std::memcmp(prev_imu, payload + imu_off, kImuSampleSize) == 0;
}

/* Bluepad Wii DRM_KA: centered int16 accel (~108 counts per G). */
inline void fill_from_wii_bt(int32_t accel[3], int32_t gyro[3], const int32_t wii_accel[3])
{
    for (int i = 0; i < 3; i++) {
        accel[i] = static_cast<int32_t>((static_cast<int64_t>(wii_accel[i]) * kDs4UnitsPerG) / kWiiAccelCountsPerG);
        gyro[i] = 0;
    }
}

/**
 * Switch motion UIs integrate gyro (Linux ABS_RX/RY/RZ), not accel (ABS_X/Y/Z).
 * Wiimote has no parsed MotionPlus gyro, so approximate rate from filtered gravity.
 *
 * hid-nintendo scales Pro gyro int16 by ~8k× (switch_raw 3 → ABS ~25k). Emit
 * instantaneous d(accel)/dt only (no accumulation), cap ±1. PadIn.gyro is
 * Switch-native int16. Still pose ⇒ gyro=0.
 */
struct WiiPseudoGyroState
{
    int32_t filt_accel[3];
    int32_t prev_filt[3];
    uint32_t prev_ms;
    bool valid;
};

inline void apply_wii_pseudo_gyro(int32_t accel[3], int32_t gyro[3], WiiPseudoGyroState& st, uint32_t now_ms)
{
    gyro[0] = gyro[1] = gyro[2] = 0;

    if (!st.valid) {
        for (int i = 0; i < 3; i++) {
            st.filt_accel[i] = accel[i];
            st.prev_filt[i] = accel[i];
        }
        st.prev_ms = now_ms;
        st.valid = true;
        return;
    }

    for (int i = 0; i < 3; i++) {
        st.filt_accel[i] = (st.filt_accel[i] * 2 + accel[i]) / 3;
        accel[i] = st.filt_accel[i];
    }

    uint32_t dt = now_ms - st.prev_ms;
    if (dt < 1) {
        return;
    }
    if (dt > 40) {
        dt = 40;
    }

    /* ±1 → Linux ~8k; d=80 over 10 ms → raw 1. */
    constexpr int32_t kMaxRaw = 1;
    for (int i = 0; i < 3; i++) {
        int32_t d = st.filt_accel[i] - st.prev_filt[i];
        st.prev_filt[i] = st.filt_accel[i];

        if (d > -20 && d < 20) {
            continue;
        }

        int32_t rate = d / (static_cast<int32_t>(dt) * 5);
        if (rate > kMaxRaw) {
            rate = kMaxRaw;
        } else if (rate < -kMaxRaw) {
            rate = -kMaxRaw;
        }
        gyro[i] = rate;
    }
    st.prev_ms = now_ms;
}

/*
 * Rotate accel/gyro into the DS4/Sixaxis playing frame (+Z = 1 G when level, sticks up).
 * Switch Pro raw: X → triggers, Y → left, Z → out of face buttons.
 * Wiimote pointed at TV: gravity mostly on -Y; map to DS4 +Z neutral.
 */
inline void remap_to_ds4_playing_frame(uint8_t motion_source, int32_t accel[3], int32_t gyro[3])
{
    const int32_t ix = accel[0];
    const int32_t iy = accel[1];
    const int32_t iz = accel[2];
    const int32_t gx = gyro[0];
    const int32_t gy = gyro[1];
    const int32_t gz = gyro[2];

    switch (motion_source) {
        case Gamepad::PadIn::MOTION_SRC_WII_BT:
            /* Wiimote IR toward TV: gravity on -Y; map into DS4 playing frame. */
            accel[0] = -ix;
            accel[1] = iz;
            accel[2] = -iy;
            gyro[0] = -gx;
            gyro[1] = gz;
            gyro[2] = -gy;
            break;
        case Gamepad::PadIn::MOTION_SRC_SWITCH_PRO:
        case Gamepad::PadIn::MOTION_SRC_SWITCH_USB:
            accel[0] = -iy;
            accel[1] = -ix;
            accel[2] = -iz;
            gyro[0] = -gy;
            gyro[1] = -gx;
            gyro[2] = -gz;
            break;
        default:
            break;
    }
}

inline int16_t clamp_i16(int32_t v)
{
    if (v > 32767) {
        return 32767;
    }
    if (v < -32768) {
        return -32768;
    }
    return static_cast<int16_t>(v);
}

/**
 * Encode PadIn motion into one Switch Pro IMU sample (little-endian int16
 * accel[3] + gyro[3]). PadIn units: Switch sources use Pro-native accel and
 * gyro*1000; DS4/DS5 use DS4 playing-frame units (8192/G); Wii uses Wiimote-frame
 * accel (DS4 units/g) and **Switch-native** gyro (see apply_wii_pseudo_gyro).
 */
inline void encode_switch_imu_sample(SwitchImuSample& out, const Gamepad::PadIn& gp_in)
{
    if (!gp_in.has_motion()) {
        out.accel[0] = 0;
        out.accel[1] = 0;
        out.accel[2] = static_cast<int16_t>(kSwitchDefaultAccelScale); /* +1 G face-up */
        out.gyro[0] = out.gyro[1] = out.gyro[2] = 0;
        return;
    }

    int32_t ax = gp_in.accel[0];
    int32_t ay = gp_in.accel[1];
    int32_t az = gp_in.accel[2];
    int32_t gx = gp_in.gyro[0];
    int32_t gy = gp_in.gyro[1];
    int32_t gz = gp_in.gyro[2];

    int32_t sx = 0;
    int32_t sy = 0;
    int32_t sz = 0;
    int32_t sgx = 0;
    int32_t sgy = 0;
    int32_t sgz = 0;

    switch (gp_in.motion_source) {
        case Gamepad::PadIn::MOTION_SRC_SWITCH_PRO:
        case Gamepad::PadIn::MOTION_SRC_SWITCH_USB:
            out.accel[0] = clamp_i16(ax);
            out.accel[1] = clamp_i16(ay);
            out.accel[2] = clamp_i16(az);
            out.gyro[0] = clamp_i16(gx / kSwitchGyroPrecScale);
            out.gyro[1] = clamp_i16(gy / kSwitchGyroPrecScale);
            out.gyro[2] = clamp_i16(gz / kSwitchGyroPrecScale);
            return;
        case Gamepad::PadIn::MOTION_SRC_WII_BT:
            /* Accel: Wiimote → Switch frame (DS4 units → Pro scale). */
            sx = -az;
            sy = ax;
            sz = ay;
            out.accel[0] = clamp_i16((sx * kSwitchDefaultAccelScale) / kDs4UnitsPerG);
            out.accel[1] = clamp_i16((sy * kSwitchDefaultAccelScale) / kDs4UnitsPerG);
            out.accel[2] = clamp_i16((sz * kSwitchDefaultAccelScale) / kDs4UnitsPerG);
            /* Gyro already Switch-native; same axis remap (no /1000). */
            out.gyro[0] = clamp_i16(-gz);
            out.gyro[1] = clamp_i16(gx);
            out.gyro[2] = clamp_i16(gy);
            return;
        default:
            /* DS4/DS5 PadIn is DS4 playing frame; invert Switch→DS4 remap. */
            sx = -ay;
            sy = -ax;
            sz = -az;
            sgx = -gy;
            sgy = -gx;
            sgz = -gz;
            break;
    }

    out.accel[0] = clamp_i16((sx * kSwitchDefaultAccelScale) / kDs4UnitsPerG);
    out.accel[1] = clamp_i16((sy * kSwitchDefaultAccelScale) / kDs4UnitsPerG);
    out.accel[2] = clamp_i16((sz * kSwitchDefaultAccelScale) / kDs4UnitsPerG);
    out.gyro[0] = clamp_i16(sgx / kSwitchGyroPrecScale);
    out.gyro[1] = clamp_i16(sgy / kSwitchGyroPrecScale);
    out.gyro[2] = clamp_i16(sgz / kSwitchGyroPrecScale);
}

/**
 * Write three IMU samples (36 bytes) at dest — Pro report 0x30 layout.
 * Real Pro spaces samples ~5 ms apart; keep a short history so hosts that
 * differentiate slots (or average them) see motion instead of three copies.
 *
 * Wii pseudo-gyro is a one-shot impulse: do **not** use history (it would
 * re-emit old gyro in older slots for 2–3 reports). Only the newest slot gets
 * gyro; the other two carry accel only.
 */
struct SwitchImuHistory
{
    SwitchImuSample samples[3];
    bool valid;
};

inline void encode_switch_imu_report(uint8_t* dest, const Gamepad::PadIn& gp_in, SwitchImuHistory& hist)
{
    SwitchImuSample sample{};
    encode_switch_imu_sample(sample, gp_in);

    if (gp_in.motion_source == Gamepad::PadIn::MOTION_SRC_WII_BT) {
        SwitchImuSample accel_only = sample;
        accel_only.gyro[0] = accel_only.gyro[1] = accel_only.gyro[2] = 0;
        std::memcpy(dest + 0 * sizeof(sample), &accel_only, sizeof(sample));
        std::memcpy(dest + 1 * sizeof(sample), &accel_only, sizeof(sample));
        std::memcpy(dest + 2 * sizeof(sample), &sample, sizeof(sample));
        hist.valid = false;
        return;
    }

    if (!hist.valid) {
        hist.samples[0] = hist.samples[1] = hist.samples[2] = sample;
        hist.valid = true;
    } else {
        hist.samples[0] = hist.samples[1];
        hist.samples[1] = hist.samples[2];
        hist.samples[2] = sample;
    }
    for (int i = 0; i < 3; i++) {
        std::memcpy(dest + static_cast<size_t>(i) * sizeof(sample), &hist.samples[i], sizeof(sample));
    }
}

} // namespace MotionImu

#endif // _MOTION_IMU_H_
