#include <cstring>
#include <algorithm>

#include "pico/time.h"
#include "Gamepad/MotionImu.h"
#include "USBDevice/DeviceDriver/PS3/PS3.h"

namespace {

/*
 * DS3 Sixaxis wire format (Linux drivers/hid/hid-sony.c): bytes 41–48 are MSB-first 16-bit values;
 * accel layout X @ 41–42, Z @ 43–44, Y @ 45–46; kernel uses 10-bit–style center 511.
 * PadIn accel/gyro follow Bluepad DS4 units (accel, gyro[2] for yaw rate).
 */
void apply_pad_imu_to_ps3_sixaxis(PS3::InReport& rep, const Gamepad::PadIn& gp_in)
{
    constexpr int32_t kDs4UnitsPerG = 8192;
    constexpr int32_t kDs3CountsPerG = 113;
    constexpr int32_t kGyroPerDps = 1024;

    int32_t accel[3] = {gp_in.accel[0], gp_in.accel[1], gp_in.accel[2]};
    int32_t gyro[3] = {gp_in.gyro[0], gp_in.gyro[1], gp_in.gyro[2]};
    MotionImu::remap_to_ds4_playing_frame(gp_in.motion_source, accel, gyro);

    int32_t ax = accel[0];
    int32_t ay = accel[1];
    int32_t az = accel[2];

    auto iabs = [](int32_t v) -> int64_t {
        const int64_t w = static_cast<int64_t>(v);
        return w >= 0 ? w : -w;
    };
    const int64_t l1 = iabs(ax) + iabs(ay) + iabs(az);
    constexpr int64_t kNearZeroL1 = 512;
    if (l1 < kNearZeroL1) {
        ax = 0;
        ay = 0;
        az = kDs4UnitsPerG;
    }

    auto ds4_to_ds3_delta = [](int32_t ds4_units) -> int32_t {
        return static_cast<int32_t>((static_cast<int64_t>(ds4_units) * kDs3CountsPerG) / kDs4UnitsPerG);
    };

    auto encode_accel = [&ds4_to_ds3_delta](int32_t ds4_units, bool kernel_inverts) -> uint16_t {
        int32_t d = ds4_to_ds3_delta(ds4_units);
        if (d > 511) {
            d = 511;
        }
        if (d < -512) {
            d = -512;
        }
        int32_t raw = kernel_inverts ? (511 - d) : (511 + d);
        if (raw < 0) {
            raw = 0;
        }
        if (raw > 1023) {
            raw = 1023;
        }
        return static_cast<uint16_t>(raw);
    };

    const bool wii_imu = gp_in.motion_source == Gamepad::PadIn::MOTION_SRC_WII_BT;

    const uint16_t raw_x = encode_accel(ax, false);
    /* Linux hid-sony: report Z bytes → kernel ABS_Z, report Y bytes → kernel ABS_Y.
     * Real DS3 neutral gravity is on ABS_Y; Wii pitch was on ay and gravity on az in
     * DS4 frame — swap wires so gravity lands on acceler_z (kernel Y). */
    uint16_t raw_z_wire;
    uint16_t raw_y_wire;
    if (wii_imu) {
        raw_z_wire = encode_accel(ay, true);
        raw_y_wire = encode_accel(az, true);
    } else {
        raw_z_wire = encode_accel(az, true);
        raw_y_wire = encode_accel(ay, true);
    }

    rep.acceler_x = __builtin_bswap16(raw_x);
    rep.acceler_y = __builtin_bswap16(raw_z_wire);
    rep.acceler_z = __builtin_bswap16(raw_y_wire);

    int32_t gz = static_cast<int32_t>((static_cast<int64_t>(gyro[2]) * 511) / (kGyroPerDps * 64));
    if (gz > 511) {
        gz = 511;
    }
    if (gz < -512) {
        gz = -512;
    }
    int32_t raw_g_i = 511 + gz;
    if (raw_g_i < 0) {
        raw_g_i = 0;
    }
    if (raw_g_i > 1023) {
        raw_g_i = 1023;
    }
    rep.gyro_z = __builtin_bswap16(static_cast<uint16_t>(raw_g_i));
}

} // namespace

void PS3Device::initialize() 
{
	class_driver_ = 
    {
		.name = TUD_DRV_NAME("PS3"),
		.init = hidd_init,
        .deinit = hidd_deinit,
		.reset = hidd_reset,
		.open = hidd_open,
		.control_xfer_cb = hidd_control_xfer_cb,
		.xfer_cb = hidd_xfer_cb,
		.sof = NULL
	};
}

void PS3Device::process(const uint8_t idx, Gamepad& gamepad) 
{
    // Remote wake when host has suspended the bus (e.g. PS3 standby): PS button or Start held 3s (same as 360).
    {
        static bool start_wake_sent = false;
        static bool start_held = false;
        static absolute_time_t start_hold_begin = { 0 };
        Gamepad::PadIn gp_wake = gamepad.get_pad_in();
        bool start_pressed = (gp_wake.buttons & Gamepad::BUTTON_START) != 0;
        if (start_pressed)
        {
            if (!start_held)
            {
                start_held = true;
                start_hold_begin = get_absolute_time();
            }
            else
            {
                uint64_t hold_ms = to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(start_hold_begin);
                if (hold_ms >= 3000 && tud_suspended() && !start_wake_sent)
                {
                    tud_remote_wakeup();
                    start_wake_sent = true;
                }
            }
        }
        else
        {
            start_held = false;
            start_wake_sent = false;
        }
        if (tud_suspended() && (gp_wake.buttons & Gamepad::BUTTON_SYS))
            tud_remote_wakeup();
    }

    /* Rebuild report_in_ every frame so GET_REPORT and IMU stay current; send on HID IN when ready. */
    {
        Gamepad::PadIn gp_in = gamepad.get_pad_in();
        report_in_ = PS3::InReport();

        switch (gp_in.dpad)
        {
            case Gamepad::DPAD_UP:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_UP;
                break;
            case Gamepad::DPAD_DOWN:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_DOWN;
                break;
            case Gamepad::DPAD_LEFT:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_LEFT;
                break;
            case Gamepad::DPAD_RIGHT:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_RIGHT;
                break;
            case Gamepad::DPAD_UP_LEFT:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_UP | PS3::Buttons0::DPAD_LEFT;
                break;
            case Gamepad::DPAD_UP_RIGHT:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_UP | PS3::Buttons0::DPAD_RIGHT;
                break;
            case Gamepad::DPAD_DOWN_LEFT:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_DOWN | PS3::Buttons0::DPAD_LEFT;
                break;
            case Gamepad::DPAD_DOWN_RIGHT:
                report_in_.buttons[0] = PS3::Buttons0::DPAD_DOWN | PS3::Buttons0::DPAD_RIGHT;
                break;
            default:
                break;
        }

        if (gp_in.buttons & Gamepad::BUTTON_X)        report_in_.buttons[1] |= PS3::Buttons1::SQUARE;
        if (gp_in.buttons & Gamepad::BUTTON_A)        report_in_.buttons[1] |= PS3::Buttons1::CROSS;
        if (gp_in.buttons & Gamepad::BUTTON_Y)        report_in_.buttons[1] |= PS3::Buttons1::TRIANGLE;
        if (gp_in.buttons & Gamepad::BUTTON_B)        report_in_.buttons[1] |= PS3::Buttons1::CIRCLE;
        if (gp_in.buttons & Gamepad::BUTTON_LB)       report_in_.buttons[1] |= PS3::Buttons1::L1;
        if (gp_in.buttons & Gamepad::BUTTON_RB)       report_in_.buttons[1] |= PS3::Buttons1::R1;
        if (gp_in.buttons & Gamepad::BUTTON_BACK)     report_in_.buttons[0] |= PS3::Buttons0::SELECT;
        if (gp_in.buttons & Gamepad::BUTTON_START)    report_in_.buttons[0] |= PS3::Buttons0::START;
        if (gp_in.buttons & Gamepad::BUTTON_L3)       report_in_.buttons[0] |= PS3::Buttons0::L3;
        if (gp_in.buttons & Gamepad::BUTTON_R3)       report_in_.buttons[0] |= PS3::Buttons0::R3;
        // DS3 report: buttons[2] byte = PS (bit0), Touchpad (bit1). Latch PS so DS4/DS5 over BT short taps aren't missed.
        if (gp_in.buttons & Gamepad::BUTTON_SYS)
            sys_button_latch_frames_ = 8;
        if (sys_button_latch_frames_ > 0) {
            report_in_.buttons[2] |= PS3::Buttons2::SYS;
            sys_button_latch_frames_--;
        }
        if (gp_in.buttons & Gamepad::BUTTON_MISC)     report_in_.buttons[2] |= PS3::Buttons2::TP;

        if (gp_in.trigger_l) report_in_.buttons[1] |= PS3::Buttons1::L2;
        if (gp_in.trigger_r) report_in_.buttons[1] |= PS3::Buttons1::R2;

        // L2/R2 axis values (critical for PS3 games - avoids stuck triggers)
        report_in_.l2_axis = gp_in.trigger_l;
        report_in_.r2_axis = gp_in.trigger_r;

        // Emulate DS3/Sixaxis sticks: 8-bit range 0–255, center 0x80 (128). Small deadzone (~1.5%) like real hardware.
        constexpr int16_t DEADZONE = 512;   // ~1.5% of 32768
        constexpr uint8_t PS3_CENTER = PS3::JOYSTICK_MID;  // 0x80

        auto joystick_to_ps3 = [](int16_t value, int16_t deadzone) -> uint8_t {
            if (value > -deadzone && value < deadzone)
                return PS3_CENTER;
            int32_t in = static_cast<int32_t>(value) + 32768;
            int32_t scaled = (in * 255 + 32768) / 65535;
            if (scaled < 0) scaled = 0;
            if (scaled > 255) scaled = 255;
            return static_cast<uint8_t>(scaled);
        };

        report_in_.joystick_lx = joystick_to_ps3(gp_in.joystick_lx, DEADZONE);
        report_in_.joystick_ly = joystick_to_ps3(gp_in.joystick_ly, DEADZONE);
        report_in_.joystick_rx = joystick_to_ps3(gp_in.joystick_rx, DEADZONE);
        report_in_.joystick_ry = joystick_to_ps3(gp_in.joystick_ry, DEADZONE);

        if (gamepad.analog_enabled())
        {
            report_in_.triangle_axis = gp_in.analog[Gamepad::ANALOG_OFF_Y];
            report_in_.circle_axis   = gp_in.analog[Gamepad::ANALOG_OFF_B];
            report_in_.cross_axis    = gp_in.analog[Gamepad::ANALOG_OFF_A];
            report_in_.square_axis   = gp_in.analog[Gamepad::ANALOG_OFF_X];
            report_in_.r1_axis = gp_in.analog[Gamepad::ANALOG_OFF_RB];
            report_in_.l1_axis = gp_in.analog[Gamepad::ANALOG_OFF_LB];
            // D-pad: use digital conversion to avoid noisy analog causing stuck inputs
            report_in_.up_axis    = (gp_in.dpad & Gamepad::DPAD_UP)    ? 0xFF : 0;
            report_in_.down_axis  = (gp_in.dpad & Gamepad::DPAD_DOWN)  ? 0xFF : 0;
            report_in_.right_axis = (gp_in.dpad & Gamepad::DPAD_RIGHT) ? 0xFF : 0;
            report_in_.left_axis  = (gp_in.dpad & Gamepad::DPAD_LEFT)  ? 0xFF : 0;
        }
        else
        {
            report_in_.up_axis       = (gp_in.dpad & Gamepad::DPAD_UP)    ? 0xFF : 0;
            report_in_.down_axis     = (gp_in.dpad & Gamepad::DPAD_DOWN)  ? 0xFF : 0;
            report_in_.right_axis    = (gp_in.dpad & Gamepad::DPAD_RIGHT) ? 0xFF : 0;
            report_in_.left_axis     = (gp_in.dpad & Gamepad::DPAD_LEFT)  ? 0xFF : 0;

            report_in_.triangle_axis = (gp_in.buttons & Gamepad::BUTTON_Y) ? 0xFF : 0;
            report_in_.circle_axis   = (gp_in.buttons & Gamepad::BUTTON_B) ? 0xFF : 0;
            report_in_.cross_axis    = (gp_in.buttons & Gamepad::BUTTON_A) ? 0xFF : 0;
            report_in_.square_axis   = (gp_in.buttons & Gamepad::BUTTON_X) ? 0xFF : 0;

            report_in_.r1_axis = (gp_in.buttons & Gamepad::BUTTON_RB) ? 0xFF : 0;
            report_in_.l1_axis = (gp_in.buttons & Gamepad::BUTTON_LB) ? 0xFF : 0;
        }

        if (gp_in.has_motion()) {
            apply_pad_imu_to_ps3_sixaxis(report_in_, gp_in);
        }
    }

    if (tud_hid_ready()) {
        tud_hid_report(0, reinterpret_cast<uint8_t*>(&report_in_), sizeof(PS3::InReport));
    }

    if (new_report_out_)
    {
        Gamepad::PadOut gp_out;
        /* Windows DInput often leaves a small non-zero large-motor byte or sets the small-motor
         * field to values other than 0/1; forwarding that blindly causes constant rumble on the BT pad. */
        constexpr uint8_t HOST_LARGE_MOTOR_DEADZONE = 12;
        uint8_t lf = report_out_.rumble.left_motor_force;
        if (lf < HOST_LARGE_MOTOR_DEADZONE) {
            lf = 0;
        }
        gp_out.rumble_l = lf;
        /* DS3 output uses 0/1 for the small motor; only 0x01 counts as on (not "any non-zero"). */
        const uint8_t rm = report_out_.rumble.right_motor_on;
        gp_out.rumble_r = (rm == 1u) ? Range::MAX<uint8_t> : 0;
        gamepad.set_pad_out(gp_out);
        new_report_out_ = false;
    }
}

uint16_t PS3Device::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    if (report_type == HID_REPORT_TYPE_INPUT) 
    {
        std::memcpy(buffer, &report_in_, sizeof(PS3::InReport));
        return sizeof(PS3::InReport);
    } 
    else if (report_type == HID_REPORT_TYPE_FEATURE) 
    {
        uint16_t resp_len = 0;
        uint8_t ctr = 0;

        switch(report_id) 
        {
            case PS3::ReportID::FEATURE_01:
                resp_len = reqlen;
                std::memcpy(buffer, PS3::OUTPUT_0x01, resp_len);
                return resp_len;
            case PS3::ReportID::FEATURE_EF:
                resp_len = reqlen;
                std::memcpy(buffer, PS3::OUTPUT_0xEF, resp_len);
                buffer[6] = ef_byte_;
                return resp_len;
            case PS3::ReportID::GET_PAIRING_INFO:
                resp_len = reqlen;
                std::memcpy(buffer, &bt_info_, resp_len);
                return resp_len;
            case PS3::ReportID::FEATURE_F5:
                resp_len = reqlen;
                std::memcpy(buffer, PS3::OUTPUT_0xF5, resp_len);
                for (ctr = 0; ctr < 6; ctr++) 
                {
                    buffer[1+ctr] = bt_info_.host_address[ctr];
                }
                return resp_len;
            case PS3::ReportID::FEATURE_F7:
                resp_len = reqlen;
                std::memcpy(buffer, PS3::OUTPUT_0xF7, resp_len);
                return resp_len;
            case PS3::ReportID::FEATURE_F8:
                resp_len = reqlen;
                std::memcpy(buffer, PS3::OUTPUT_0xF8, resp_len);
                buffer[6] = ef_byte_;
                return resp_len;
        }
    }
    return 0;
}

void PS3Device::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) 
{
    if (report_type == HID_REPORT_TYPE_FEATURE) 
    {
        switch(report_id) 
        {
            case PS3::ReportID::FEATURE_EF:
                ef_byte_ = buffer[6];
                break;
        }
    } 
    else if (report_type == HID_REPORT_TYPE_OUTPUT) 
    {
        // DS3 command
        uint8_t const *buf = buffer;
        if (report_id == 0 && bufsize > 0) 
        {
            report_id = buffer[0];
            bufsize = bufsize - 1;
            buf = &buffer[1];
        }
        switch(report_id) 
        {
            case PS3::ReportID::FEATURE_01:
                new_report_out_ = true;
                std::memcpy(&report_out_, buf, std::min(bufsize, static_cast<uint16_t>(sizeof(PS3::OutReport))));
                break;
        }
    }
}

bool PS3Device::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return false;
}

const uint16_t* PS3Device::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	const char *value = reinterpret_cast<const char*>(PS3::STRING_DESCRIPTORS[index]);
	return get_string_descriptor(value, index);
}

const uint8_t* PS3Device::get_descriptor_device_cb() 
{
    return PS3::DEVICE_DESCRIPTORS;
}

const uint8_t* PS3Device::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return PS3::REPORT_DESCRIPTORS;
}

const uint8_t* PS3Device::get_descriptor_configuration_cb(uint8_t index) 
{
    return PS3::CONFIGURATION_DESCRIPTORS;
}

const uint8_t* PS3Device::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}