#include <array>
#include <algorithm>
#include <atomic>
#include <pico/stdlib.h>
#include <hardware/timer.h>
#include <hardware/irq.h>
#include <hardware/sync.h>

#include "Board/board_api.h"
#include "board_config.h"
#include "TaskQueue/TaskQueue.h"

namespace TaskQueue {

class CoreQueue
{
public:
    enum class CoreNum : uint8_t
    {
        Core0 = 0,
        Core1,
    };

    CoreQueue(CoreNum core_num) 
        :   core_num_(core_num)
    {   
        uint32_t idx = (core_num_ == CoreNum::Core0) ? 0 : 1;
#if (OGXM_BOARD != PI_PICOW)
        alarm_num_ = idx;
#else
        alarm_num_ = idx + 1; //BTStack uses alarm 0
#endif
        instances_[idx] = this;

        hw_set_bits(&timer_hw->inte, 1u << alarm_num_);

        irq_set_exclusive_handler(
            TIMER_IRQ(alarm_num_), 
            (core_num_ == CoreNum::Core0) ? timer_irq_wrapper_c0 : timer_irq_wrapper_c1);

        irq_set_enabled(TIMER_IRQ(alarm_num_), true);
    }

    ~CoreQueue() = default;

    uint32_t get_new_task_id()
    {
        return new_task_id_++;
    }

    bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function)
    {
        uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);
        for (const auto& task : task_queue_delayed_) 
        {
            if (task.task_id == task_id)
            {
                spin_unlock(spinlock_delayed_, irq_state);
                return false;
            }
        }

        hw_set_bits(&timer_hw->inte, 1u << alarm_num_);

        //Might come back to this, there is overflow potential
        uint64_t target_time = timer_hw->timerawl + static_cast<uint64_t>(delay_ms) * 1000;

        for (auto& task : task_queue_delayed_) 
        {
            if (!task.function) 
            {
                task.target_time = target_time;
                task.interval_ms = repeating ? delay_ms : 0;
                task.function = function;
                task.task_id = task_id;

                auto it = std::min_element(task_queue_delayed_.begin(), task_queue_delayed_.end(), [](const DelayedTask& a, const DelayedTask& b) 
                {
                    return a.function && (!b.function || a.target_time < b.target_time);
                });

                if (it != task_queue_delayed_.end() && it->function) 
                {
                    timer_hw->alarm[alarm_num_] = static_cast<uint32_t>(it->target_time);
                }

                spin_unlock(spinlock_delayed_, irq_state);
                return true;
            }
        }

        spin_unlock(spinlock_delayed_, irq_state);
        return false;
    }

    void cancel_delayed_task(uint32_t task_id)
    {
        uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);
        bool found = false;

        for (auto& task : task_queue_delayed_) 
        {
            if (task.task_id == task_id) 
            {
                task.function = nullptr;
                task.task_id = 0;
                task.interval_ms = 0;
                found = true;
            }
        }

        if (!found)
        {
            spin_unlock(spinlock_delayed_, irq_state);
            return;
        }

        hw_set_bits(&timer_hw->inte, 1u << alarm_num_);

        int64_t next_target_time = get_next_target_time_unsafe(task_queue_delayed_);
        if (next_target_time >= 0) 
        {
            timer_hw->alarm[alarm_num_] = static_cast<uint32_t>(next_target_time);
        }
        
        spin_unlock(spinlock_delayed_, irq_state);
    }

    bool queue_task(const std::function<void()>& function)
    {
        uint32_t irq_state = spin_lock_blocking(spinlock_queue_);
        for (auto& task : task_queue_)
        {
            if (!task.function)
            {
                task.function = function;
                spin_unlock(spinlock_queue_, irq_state);
                return true;
            }
        }
        spin_unlock(spinlock_queue_, irq_state);
        return false;
    }

    void process_tasks()
    {
        uint32_t irq_state = spin_lock_blocking(spinlock_queue_);
        for (auto& task : task_queue_)
        {
            if (task.function)
            {
                auto function = task.function;
                task.function = nullptr;
                spin_unlock(spinlock_queue_, irq_state);

                function();

                irq_state = spin_lock_blocking(spinlock_queue_);
            }
            else
            {
                break; //No more tasks
            }
        }
        spin_unlock(spinlock_queue_, irq_state);
    }

private:
    CoreQueue& operator=(const CoreQueue&) = delete;
    CoreQueue(CoreQueue&&) = delete;
    CoreQueue& operator=(CoreQueue&&) = delete;

    static CoreQueue* instances_[static_cast<uint8_t>(CoreNum::Core1) + 1];

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

    CoreNum core_num_;
    uint32_t alarm_num_;
    uint32_t new_task_id_ = 1;

    int spinlock_queue_num_ = spin_lock_claim_unused(true);
    int spinlock_delayed_num_ = spin_lock_claim_unused(true);
    spin_lock_t* spinlock_queue_ = spin_lock_instance(static_cast<uint>(spinlock_queue_num_));
    spin_lock_t* spinlock_delayed_ = spin_lock_instance(static_cast<uint>(spinlock_delayed_num_));

    std::array<Task, MAX_TASKS> task_queue_;
    std::array<DelayedTask, MAX_DELAYED_TASKS> task_queue_delayed_;

    static void timer_irq_wrapper_c0()
    {
        instances_[static_cast<uint8_t>(CoreNum::Core0)]->timer_irq_handler();
    }

    static void timer_irq_wrapper_c1()
    {
        instances_[static_cast<uint8_t>(CoreNum::Core1)]->timer_irq_handler();
    }
    
    static inline uint32_t TIMER_IRQ(uint32_t alarm_num)
    {
        // switch (alarm_num)
        // {
        //     case 1:
        //         return TIMER_IRQ_1;
        //     case 2:
        //         return TIMER_IRQ_2;
        //     default:
        //         return TIMER_IRQ_0;
        // }
        
        return timer_hardware_alarm_get_irq_num(timer_hw, alarm_num);
    }

    static inline uint64_t get_time_64_us()
    {
        static spin_lock_t* spinlock_time_ = nullptr;
        if (!spinlock_time_)
        {
            int spinlock_time_num = spin_lock_claim_unused(true);
            spinlock_time_ = spin_lock_instance(static_cast<uint>(spinlock_time_num));
        }
        uint32_t irq_state = spin_lock_blocking(spinlock_time_);
        uint32_t lo = timer_hw->timelr;
        uint32_t hi = timer_hw->timehr;
        spin_unlock(spinlock_time_, irq_state);
        return ((uint64_t) hi << 32u) | lo;;
    }

    void timer_irq_handler()
    {
        hw_clear_bits(&timer_hw->intr, 1u << alarm_num_);

        uint64_t now = get_time_64_us();
        uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);

        for (auto& task : task_queue_delayed_) 
        {
            if (task.function && task.target_time <= now) 
            {
                auto function = task.function;
                if (task.interval_ms) 
                {
                    task.target_time += (task.interval_ms * 1000);
                } 
                else 
                {
                    task.function = nullptr;
                    task.task_id = 0;
                }

                spin_unlock(spinlock_delayed_, irq_state);
                queue_task(function);
                irq_state = spin_lock_blocking(spinlock_delayed_);
            }
        }

        int64_t next_target_time = get_next_target_time_unsafe(task_queue_delayed_);
        if (next_target_time >= 0) 
        {
            timer_hw->alarm[alarm_num_] = static_cast<uint32_t>(next_target_time);
        }

        spin_unlock(spinlock_delayed_, irq_state);
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
};

CoreQueue* CoreQueue::instances_[static_cast<uint8_t>(CoreQueue::CoreNum::Core1) + 1]{nullptr};

namespace Core0
{
    static inline CoreQueue& get_core_0()
    {
        static CoreQueue core(CoreQueue::CoreNum::Core0);
        return core;
    }
    uint32_t get_new_task_id()
    {
        return get_core_0().get_new_task_id();
    }
    void cancel_delayed_task(uint32_t task_id)
    {
        get_core_0().cancel_delayed_task(task_id);
    }
    bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function)
    {
        return get_core_0().queue_delayed_task(task_id, delay_ms, repeating, function);
    }
    bool queue_task(const std::function<void()>& function)
    {
        return get_core_0().queue_task(function);
    }
    void process_tasks()
    {
        get_core_0().process_tasks();
    }
}

#if OGXM_BOARD != PI_PICOW

namespace Core1
{
    static inline CoreQueue& get_core_1()
    {
        static CoreQueue core(CoreQueue::CoreNum::Core1);
        return core;
    }
    uint32_t get_new_task_id()
    {
        return get_core_1().get_new_task_id();
    }
    void cancel_delayed_task(uint32_t task_id)
    {
        get_core_1().cancel_delayed_task(task_id);
    }
    bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function)
    {
        return get_core_1().queue_delayed_task(task_id, delay_ms, repeating, function);
    }
    bool queue_task(const std::function<void()>& function)
    {
        return get_core_1().queue_task(function);
    }
    void process_tasks()
    {
        get_core_1().process_tasks();
    }
}

#endif // OGXM_BOARD != PI_PICOW

} // namespace TaskQueue

// #include <array>
// #include <algorithm>
// #include <pico/stdlib.h>
// #include <hardware/timer.h>
// #include <hardware/irq.h>
// #include <hardware/sync.h>

// #include "Board/board_api.h"
// #include "TaskQueue/TaskQueue.h"

// namespace TaskQueue {

// struct Task
// {
//     // uint32_t task_id = 0;
//     std::function<void()> function = nullptr;
// };

// struct DelayedTask
// {
//     uint32_t task_id = 0;
//     uint32_t interval_ms = 0;
//     uint64_t target_time = 0;
//     std::function<void()> function = nullptr;
// };

// static constexpr uint8_t MAX_TASKS = 8;
// static constexpr uint8_t MAX_DELAYED_TASKS = MAX_TASKS * 2;

// static inline uint32_t TIMER_IRQ(int alarm_num)
// {
//     return timer_hardware_alarm_get_irq_num(timer_hw, alarm_num);
// }

// static uint64_t get_time_64_us()
// {
//     static spin_lock_t* spinlock_64_us = spin_lock_instance(4);
//     uint32_t irq_state = spin_lock_blocking(spinlock_64_us);
//     uint32_t lo = timer_hw->timelr;
//     uint32_t hi = timer_hw->timehr;
//     spin_unlock(spinlock_64_us, irq_state);
//     return ((uint64_t) hi << 32u) | lo;;
// }

// #if OGXM_BOARD != PI_PICOW

// namespace Core1 {

//     static constexpr uint32_t ALARM_NUM = 1;

//     static std::array<Task, MAX_TASKS> tasks_;
//     static std::array<DelayedTask, MAX_DELAYED_TASKS> delayed_tasks_;

//     static bool delay_irq_set_ = false;
//     static uint32_t new_task_id_ = 1;

//     static spin_lock_t* spinlock_ = spin_lock_instance(1);
//     static spin_lock_t* spinlock_delayed_ = spin_lock_instance(2);

//     bool queue_task(const std::function<void()>& function); 

//     uint32_t get_new_task_id()
//     {
//         return new_task_id_++;
//     }

//     static void timer_irq_handler()
//     {
//         hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

//         uint64_t now = get_time_64_us();
//         uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);

//         for (auto& task : delayed_tasks_) 
//         {
//             if (task.function && task.target_time <= now) 
//             {
//                 auto function = task.function;
//                 if (task.interval_ms) 
//                 {
//                     task.target_time += (task.interval_ms * 1000);
//                 } 
//                 else 
//                 {
//                     task.function = nullptr;
//                     task.task_id = 0;
//                 }

//                 spin_unlock(spinlock_delayed_, irq_state);
//                 queue_task(function);
//                 irq_state = spin_lock_blocking(spinlock_delayed_);
//             }
//         }

//         auto it = std::min_element(delayed_tasks_.begin(), delayed_tasks_.end(), [](const DelayedTask& a, const DelayedTask& b) 
//         {
//             //Get task with the earliest target time
//             return a.function && (!b.function || a.target_time < b.target_time);
//         });

//         if (it != delayed_tasks_.end() && it->function) 
//         {
//             timer_hw->alarm[ALARM_NUM] = static_cast<uint32_t>(it->target_time);
//         }

//         spin_unlock(spinlock_delayed_, irq_state);
//     }

//     bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function)
//     {
//         uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);
//         for (const auto& task : delayed_tasks_) 
//         {
//             if (task.task_id == task_id)
//             {
//                 spin_unlock(spinlock_delayed_, irq_state);
//                 return false;
//             }
//         }
//         spin_unlock(spinlock_delayed_, irq_state);

//         hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
        
//         if (!delay_irq_set_)
//         {
//             delay_irq_set_ = true;
//             irq_set_exclusive_handler(TIMER_IRQ(ALARM_NUM), timer_irq_handler);
//             irq_set_enabled(TIMER_IRQ(ALARM_NUM), true);
//         }

//         irq_state = spin_lock_blocking(spinlock_delayed_);
        
//         //Might come back to this, there is overflow potential
//         uint64_t target_time = timer_hw->timerawl + static_cast<uint64_t>(delay_ms) * 1000;

//         for (auto& task : delayed_tasks_) 
//         {
//             if (!task.function) 
//             {
//                 task.target_time = target_time;
//                 task.interval_ms = repeating ? delay_ms : 0;
//                 task.function = function;
//                 task.task_id = task_id;

//                 auto it = std::min_element(delayed_tasks_.begin(), delayed_tasks_.end(), [](const DelayedTask& a, const DelayedTask& b) 
//                 {
//                     return a.function && (!b.function || a.target_time < b.target_time);
//                 });

//                 if (it != delayed_tasks_.end() && it->function) 
//                 {
//                     timer_hw->alarm[ALARM_NUM] = static_cast<uint32_t>(it->target_time);
//                 }

//                 spin_unlock(spinlock_delayed_, irq_state);
//                 return true;
//             }
//         }

//         spin_unlock(spinlock_delayed_, irq_state);
//         return false;
//     }

//     void cancel_delayed_task(uint32_t task_id)
//     {
//         uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);
//         bool found = false;

//         for (auto& task : delayed_tasks_) 
//         {
//             if (task.task_id == task_id) 
//             {
//                 task.function = nullptr;
//                 task.task_id = 0;
//                 found = true;
//             }
//         }

//         if (!found || !delay_irq_set_)
//         {
//             spin_unlock(spinlock_delayed_, irq_state);
//             return;
//         }

//         hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);

//         auto it = std::min_element(delayed_tasks_.begin(), delayed_tasks_.end(), [](const DelayedTask& a, const DelayedTask& b) 
//         {
//             return a.function && (!b.function || a.target_time < b.target_time);
//         });

//         if (it != delayed_tasks_.end() && it->function) 
//         {
//             timer_hw->alarm[ALARM_NUM] = static_cast<uint32_t>(it->target_time);
//         }
        
//         spin_unlock(spinlock_delayed_, irq_state);
//     }

//     bool queue_task(const std::function<void()>& function)
//     {
//         uint32_t irq_state = spin_lock_blocking(spinlock_);
//         for (auto& task : tasks_)
//         {
//             if (!task.function)
//             {
//                 task.function = function;
//                 spin_unlock(spinlock_, irq_state);
//                 return true;
//             }
//         }
//         spin_unlock(spinlock_, irq_state);
//         return false;
//     }

//     void process_tasks()
//     {
//         uint32_t irq_state = spin_lock_blocking(spinlock_);
//         for (auto& task : tasks_)
//         {
//             if (task.function)
//             {
//                 auto function = task.function;
//                 task.function = nullptr;
//                 spin_unlock(spinlock_, irq_state);

//                 function();

//                 irq_state = spin_lock_blocking(spinlock_);
//             }
//             else
//             {
//                 break; //No more tasks
//             }
//         }
//         spin_unlock(spinlock_, irq_state);
//     }

// } // namespace Core1

// #endif // OGXM_BOARD != PI_PICOW

// namespace Core0 {

// #if OGXM_BOARD == PI_PICOW
//     static constexpr uint32_t ALARM_NUM = 1;
// #else
//     static constexpr uint32_t ALARM_NUM = 0;
// #endif

//     static std::array<Task, MAX_TASKS> tasks_;
//     static std::array<DelayedTask, MAX_DELAYED_TASKS> delayed_tasks_;

//     static bool delay_irq_set_ = false;
//     static uint32_t new_task_id_ = 1;

//     static spin_lock_t* spinlock_ = spin_lock_instance(3);
//     static spin_lock_t* spinlock_delayed_ = spin_lock_instance(4);

//     uint32_t get_new_task_id()
//     {
//         return new_task_id_++;
//     }

//     static void timer_irq_handler()
//     {
//         hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

//         uint64_t now = get_time_64_us();
//         uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);

//         for (auto& task : delayed_tasks_) 
//         {
//             if (task.function && task.target_time <= now) 
//             {
//                 auto function = task.function;
//                 if (task.interval_ms) 
//                 {
//                     task.target_time += (task.interval_ms * 1000);
//                 } 
//                 else 
//                 {
//                     task.function = nullptr;
//                     task.task_id = 0;
//                 }

//                 spin_unlock(spinlock_delayed_, irq_state);
//                 queue_task(function);
//                 irq_state = spin_lock_blocking(spinlock_delayed_);
//             }
//         }

//         auto it = std::min_element(delayed_tasks_.begin(), delayed_tasks_.end(), [](const DelayedTask& a, const DelayedTask& b) 
//         {
//             return a.function && (!b.function || a.target_time < b.target_time);
//         });

//         if (it != delayed_tasks_.end() && it->function) 
//         {
//             timer_hw->alarm[ALARM_NUM] = static_cast<uint32_t>(it->target_time);
//         }

//         spin_unlock(spinlock_delayed_, irq_state);
//     }

//     bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function)
//     {
//         uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);
//         for (const auto& task : delayed_tasks_) 
//         {
//             if (task.task_id == task_id)
//             {
//                 spin_unlock(spinlock_delayed_, irq_state);
//                 return false;
//             }
//         }
//         spin_unlock(spinlock_delayed_, irq_state);

//         hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
        
//         if (!delay_irq_set_)
//         {
//             delay_irq_set_ = true;
//             irq_set_exclusive_handler(TIMER_IRQ(ALARM_NUM), timer_irq_handler);
//             irq_set_enabled(TIMER_IRQ(ALARM_NUM), true);
//         }

//         irq_state = spin_lock_blocking(spinlock_delayed_);
        
//         uint64_t target_time = timer_hw->timerawl + static_cast<uint64_t>(delay_ms) * 1000;

//         for (auto& task : delayed_tasks_) 
//         {
//             if (!task.function) 
//             {
//                 task.target_time = target_time;
//                 task.interval_ms = repeating ? delay_ms : 0;
//                 task.function = function;
//                 task.task_id = task_id;

//                 auto it = std::min_element(delayed_tasks_.begin(), delayed_tasks_.end(), [](const DelayedTask& a, const DelayedTask& b) 
//                 {
//                     return a.function && (!b.function || a.target_time < b.target_time);
//                 });

//                 if (it != delayed_tasks_.end() && it->function) 
//                 {
//                     timer_hw->alarm[ALARM_NUM] = static_cast<uint32_t>(it->target_time);
//                 }

//                 spin_unlock(spinlock_delayed_, irq_state);
//                 return true;
//             }
//         }

//         spin_unlock(spinlock_delayed_, irq_state);
//         return false;
//     }

//     void cancel_delayed_task(uint32_t task_id)
//     {
//         uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);
//         bool found = false;

//         for (auto& task : delayed_tasks_) 
//         {
//             if (task.task_id == task_id) 
//             {
//                 task.function = nullptr;
//                 task.task_id = 0;
//                 found = true;
//             }
//         }

//         if (!found || !delay_irq_set_)
//         {
//             spin_unlock(spinlock_delayed_, irq_state);
//             return;
//         }

//         hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);

//         auto it = std::min_element(delayed_tasks_.begin(), delayed_tasks_.end(), [](const DelayedTask& a, const DelayedTask& b) 
//         {
//             return a.function && (!b.function || a.target_time < b.target_time);
//         });

//         if (it != delayed_tasks_.end() && it->function) 
//         {
//             timer_hw->alarm[ALARM_NUM] = static_cast<uint32_t>(it->target_time);
//         }
        
//         spin_unlock(spinlock_delayed_, irq_state);
//     }

//     bool queue_task(const std::function<void()>& function)
//     {
//         uint32_t irq_state = spin_lock_blocking(spinlock_);
//         for (auto& task : tasks_)
//         {
//             if (!task.function)
//             {
//                 task.function = function;
//                 spin_unlock(spinlock_, irq_state);
//                 return true;
//             }
//         }
//         spin_unlock(spinlock_, irq_state);
//         return false;
//     }

//     void process_tasks()
//     {
//         uint32_t irq_state = spin_lock_blocking(spinlock_);
//         for (auto& task : tasks_)
//         {
//             if (task.function)
//             {
//                 auto function = task.function;
//                 task.function = nullptr;
//                 spin_unlock(spinlock_, irq_state);

//                 function();

//                 irq_state = spin_lock_blocking(spinlock_);
//             }
//             else
//             {
//                 break; //No more tasks
//             }
//         }
//         spin_unlock(spinlock_, irq_state);
//     }

// } // namespace Core0

// } // namespace TaskQueue