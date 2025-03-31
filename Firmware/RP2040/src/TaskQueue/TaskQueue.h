#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <pico/stdlib.h>
#include <hardware/timer.h>
#include <hardware/irq.h>
#include <hardware/sync.h>

#include "Board/Config.h"

class TaskQueue
{
public:
    struct Core0
    {
        static inline uint32_t get_new_task_id()
        {
            return get_core0().get_new_task_id();
        }
        static inline void cancel_delayed_task(uint32_t task_id)
        {
            get_core0().cancel_delayed_task(task_id);
        }
        static inline bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function)
        {
            return get_core0().queue_delayed_task(task_id, delay_ms, repeating, function);
        }
        static inline bool queue_task(const std::function<void()>& function)
        {
            return get_core0().queue_task(function);
        }
        static inline void process_tasks()
        {
            get_core0().process_tasks();
        }
        static inline void suspend_delayed_tasks()
        {
            get_core0().suspend_delayed();
        }
        static inline void resume_delayed_tasks()
        {
            get_core0().resume_delayed();
        }
    };

#if (OGXM_BOARD != PI_PICOW) //BTstack uses core1
    struct Core1
    {
        static inline uint32_t get_new_task_id()
        {
            return get_core1().get_new_task_id();
        }
        static inline void cancel_delayed_task(uint32_t task_id)
        {
            get_core1().cancel_delayed_task(task_id);
        }
        static inline bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function)
        {
            return get_core1().queue_delayed_task(task_id, delay_ms, repeating, function);
        }
        static inline bool queue_task(const std::function<void()>& function)
        {
            return get_core1().queue_task(function);
        }
        static inline void process_tasks()
        {
            get_core1().process_tasks();
        }
        static inline void suspend_delayed_tasks()
        {
            get_core1().suspend_delayed();
        }
        static inline void resume_delayed_tasks()
        {
            get_core1().resume_delayed();
        }
    }; // Core1
#endif // OGXM_BOARD != PI_PICOW

    static void suspend_delayed_tasks();
    static void resume_delayed_tasks();

private:
    enum class CoreNum : uint8_t
    {
        Core0 = 0,
        Core1 = 1
    };

    TaskQueue& operator=(const TaskQueue&) = delete;
    TaskQueue(TaskQueue&&) = delete;
    TaskQueue& operator=(TaskQueue&&) = delete;

    TaskQueue(CoreNum core_num);
    ~TaskQueue() = default;

    struct Task
    {
        // uint32_t task_id = 0;
        std::function<void()> function = nullptr;
    };

    struct DelayedTask
    {
        uint32_t task_id = 0;
        uint32_t interval_ms = 0;
        uint64_t target_time = 0;
        std::function<void()> function = nullptr;
    };

    static constexpr uint8_t MAX_TASKS = 8;
    static constexpr uint8_t MAX_DELAYED_TASKS = MAX_TASKS * 2;

    // CoreNum core_num_;
    uint32_t alarm_num_;
    uint32_t new_task_id_ = 1;

    bool suspended_ = false;
    uint64_t suspended_time_ = 0;

    int spinlock_queue_num_ = spin_lock_claim_unused(true);
    int spinlock_delayed_num_ = spin_lock_claim_unused(true);
    spin_lock_t* spinlock_queue_ = spin_lock_instance(static_cast<uint>(spinlock_queue_num_));
    spin_lock_t* spinlock_delayed_ = spin_lock_instance(static_cast<uint>(spinlock_delayed_num_));

    std::array<Task, MAX_TASKS> task_queue_;
    std::array<DelayedTask, MAX_DELAYED_TASKS> task_queue_delayed_;

    static TaskQueue& get_core0()
    {
        static TaskQueue core(CoreNum::Core0);
        return core;
    }
    static TaskQueue& get_core1()
    {
        static TaskQueue core(CoreNum::Core1);
        return core;
    }

    uint32_t get_new_task_id();
    bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function);
    void cancel_delayed_task(uint32_t task_id);
    bool queue_task(const std::function<void()>& function);
    void process_tasks();

    void suspend_delayed();
    void resume_delayed();
    void timer_irq_handler();
    static uint64_t get_time_64_us();

    static inline void timer_irq_wrapper_c0()
    {
        get_core0().timer_irq_handler();
    }
    static inline void timer_irq_wrapper_c1()
    {
        get_core1().timer_irq_handler();
    }
    static inline uint32_t TIMER_IRQ(uint32_t alarm_num)
    {
        return timer_hardware_alarm_get_irq_num(timer_hw, alarm_num);
    }
    static inline int64_t get_next_target_time_unsafe(std::array<DelayedTask, MAX_DELAYED_TASKS>& task_queue_delayed)
    {
        auto it = std::min_element(task_queue_delayed.begin(), task_queue_delayed.end(), [](const DelayedTask& a, const DelayedTask& b) 
        {
            //Get task with the earliest target time
            return a.function && (!b.function || a.target_time < b.target_time);
        });

        if (it != task_queue_delayed.end() && it->function) 
        {
            return static_cast<int64_t>(it->target_time);
        }
        return -1;
    }

}; // class TaskQueue

#endif // TASK_QUEUE_H