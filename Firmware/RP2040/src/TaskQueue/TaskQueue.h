#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <cstdint>
#include <functional>

/* Queue tasks to be executed with process_tasks() */
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

    namespace Core1 
    {
        uint32_t get_new_task_id();
        void cancel_delayed_task(uint32_t task_id);
        bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function);
        bool queue_task(const std::function<void()>& function);
        void process_tasks();
    }

} // namespace TaskQueue

#endif // TASK_QUEUE_H