#ifndef _REPORTS_H_
#define _REPORTS_H_

#include <cstdint>
// #include <array>
#include <cstring>
// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"

enum class ReportID : uint8_t { UNKNOWN = 0, SET_PAD, GET_PAD };

#pragma pack(push, 1)
struct ReportIn
{
    uint8_t report_len;
    uint8_t report_id;
    uint8_t index;
    uint8_t dpad;
    uint16_t buttons;
    uint8_t trigger_l;
    uint8_t trigger_r;
    int16_t joystick_lx;
    int16_t joystick_ly;
    int16_t joystick_rx;
    int16_t joystick_ry;

    ReportIn()
    {
        std::memset(this, 0, sizeof(ReportIn));
        report_len = sizeof(ReportIn);
        report_id = static_cast<uint8_t>(ReportID::SET_PAD);
    }
};
static_assert(sizeof(ReportIn) == 16, "ReportIn is misaligned");

struct ReportOut
{
    uint8_t report_len;
    uint8_t report_id;
    uint8_t index;
    uint8_t rumble_l;
    uint8_t rumble_r;

    ReportOut()
    {
        std::memset(this, 0, sizeof(ReportOut));
        report_len = sizeof(ReportOut);
        report_id = static_cast<uint8_t>(ReportID::GET_PAD);
    }
};
static_assert(sizeof(ReportOut) == 5, "ReportOut is misaligned");
#pragma pack(pop)

// class ReportQueue
// {
// public:
//     ReportQueue()
//     {
//         report_in_mutex_ = xSemaphoreCreateMutex();
//         report_out_mutex_ = xSemaphoreCreateMutex();
//     }

//     ~ReportQueue()
//     {
//         vSemaphoreDelete(report_in_mutex_);
//         vSemaphoreDelete(report_out_mutex_);
//     }

//     void set_report_in(const ReportIn* report)
//     {
//         if (xSemaphoreTake(report_in_mutex_, portMAX_DELAY))
//         {
//             std::memcpy(&report_in_, report, sizeof(ReportIn));
//             new_report_in_ = true;
//             xSemaphoreGive(report_in_mutex_);
//         }
//     }

//     void set_report_out(const ReportOut* report)
//     {
//         if (xSemaphoreTake(report_out_mutex_, portMAX_DELAY))
//         {
//             std::memcpy(&report_out_, report, sizeof(ReportOut));
//             new_report_out_ = true;
//             xSemaphoreGive(report_out_mutex_);
//         }
//     }

//     //Return false if no new data
//     bool get_report_in(ReportIn* report)
//     {
//         if (xSemaphoreTake(report_in_mutex_, portMAX_DELAY))
//         {
//             if (new_report_in_)
//             {
//                 std::memcpy(report, &report_in_, sizeof(ReportIn));
//                 new_report_in_ = false;
//                 xSemaphoreGive(report_in_mutex_);
//                 return true;
//             }
//             xSemaphoreGive(report_in_mutex_);
//         }
//         return false;
//     }

//     //Return false if no new data
//     bool get_report_out(ReportOut* report)
//     {
//         if (xSemaphoreTake(report_out_mutex_, portMAX_DELAY))
//         {
//             if (new_report_out_)
//             {
//                 std::memcpy(report, &report_out_, sizeof(ReportOut));
//                 new_report_out_ = false;
//                 xSemaphoreGive(report_out_mutex_);
//                 return true;
//             }
//             xSemaphoreGive(report_out_mutex_);
//         }
//         return false;
//     }

// private:
//     SemaphoreHandle_t report_in_mutex_;
//     SemaphoreHandle_t report_out_mutex_;

//     ReportIn report_in_;
//     ReportOut report_out_;

//     bool new_report_in_{false};
//     bool new_report_out_{false};
// };

#endif // _REPORTS_H_