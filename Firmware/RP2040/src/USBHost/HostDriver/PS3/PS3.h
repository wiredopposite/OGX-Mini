#ifndef _PS3_HOST_H_
#define _PS3_HOST_H_

#include <cstdint>
#include <array>

#include "tusb.h"

#include "Descriptors/PS3.h"
#include "USBHost/HostDriver/HostDriver.h"

class PS3Host : public HostDriver
{
public:
    PS3Host(uint8_t idx)
        : HostDriver(idx) {}

    void initialize(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report_desc, uint16_t desc_len) override;
    void process_report(Gamepad& gamepad, uint8_t address, uint8_t instance, const uint8_t* report, uint16_t len) override;
    bool send_feedback(Gamepad& gamepad, uint8_t address, uint8_t instance) override;

private:
    enum class InitStage { RESP1, RESP2, RESP3, DONE };

    struct InitState
    {
        uint8_t dev_addr{0xFF};
        InitStage stage{InitStage::RESP1};
        std::array<uint8_t, 17> init_buffer;
        PS3::OutReport* out_report{nullptr};
        bool reports_enabled{false};
    };

    static const tusb_control_request_t RUMBLE_REQUEST;

    PS3::InReport prev_in_report_;
    PS3::OutReport out_report_;
    InitState init_state_;

    static bool send_control_xfer(uint8_t dev_addr, const tusb_control_request_t* req, uint8_t* buffer, tuh_xfer_cb_t complete_cb, uintptr_t user_data);
    static void get_report_complete_cb(tuh_xfer_s *xfer);
};

#endif // _PS3_HOST_H_