#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <cstdint>
#include <functional>

#include "board_config.h"

/*  Queue tasks to be executed with process_tasks() in the main loop on either core.
    Don't use this on the core running BTStack, that is not safe. */
namespace TaskQueue 
{
    namespace Core0 
    {
        uint32_t get_new_task_id();
        void cancel_delayed_task(uint32_t task_id);
        bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function);
        bool queue_task(const std::function<void()>& function);
        void process_tasks();
    }

#if OGXM_BOARD != PI_PICOW
    namespace Core1 
    {
        uint32_t get_new_task_id();
        void cancel_delayed_task(uint32_t task_id);
        bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function);
        bool queue_task(const std::function<void()>& function);
        void process_tasks();
    }
#endif // OGXM_BOARD != PI_PICOW

} // namespace TaskQueue

#endif // TASK_QUEUE_H