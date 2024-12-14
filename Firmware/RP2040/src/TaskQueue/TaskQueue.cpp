#include <array>
#include <algorithm>
#include <pico/stdlib.h>
#include <hardware/timer.h>
#include <hardware/irq.h>
#include <hardware/sync.h>

#include "Board/board_api.h"
#include "TaskQueue/TaskQueue.h"

namespace TaskQueue {

struct Task
{
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

static inline uint32_t TIMER_IRQ(int alarm_num)
{
    return timer_hardware_alarm_get_irq_num(timer_hw, alarm_num);
}

static uint64_t get_time_64_us()
{
    static spin_lock_t* spinlock_64_us = spin_lock_instance(4);
    uint32_t irq_state = spin_lock_blocking(spinlock_64_us);
    uint32_t lo = timer_hw->timelr;
    uint32_t hi = timer_hw->timehr;
    spin_unlock(spinlock_64_us, irq_state);
    return ((uint64_t) hi << 32u) | lo;;
}

namespace Core1 {

    static constexpr uint32_t CORE = 1;
    static constexpr uint32_t ALARM_NUM = CORE;

    static std::array<Task, MAX_TASKS> tasks_;
    static std::array<DelayedTask, MAX_DELAYED_TASKS> delayed_tasks_;

    static bool delay_irq_set_ = false;
    static uint32_t new_task_id_ = 1;

    static spin_lock_t* spinlock_ = spin_lock_instance(CORE);
    static spin_lock_t* spinlock_delayed_ = spin_lock_instance(CORE + 2);

    uint32_t get_new_task_id()
    {
        return new_task_id_++;
    }

    static void timer_irq_handler()
    {
        hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

        uint64_t now = get_time_64_us();
        uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);

        for (auto& task : delayed_tasks_) 
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

        auto it = std::min_element(delayed_tasks_.begin(), delayed_tasks_.end(), [](const DelayedTask& a, const DelayedTask& b) 
        {
            return a.function && (!b.function || a.target_time < b.target_time);
        });

        if (it != delayed_tasks_.end() && it->function) 
        {
            timer_hw->alarm[ALARM_NUM] = static_cast<uint32_t>(it->target_time);
        }

        spin_unlock(spinlock_delayed_, irq_state);
    }

    bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function)
    {
        uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);
        for (const auto& task : delayed_tasks_) 
        {
            if (task.task_id == task_id)
            {
                spin_unlock(spinlock_delayed_, irq_state);
                return false;
            }
        }
        spin_unlock(spinlock_delayed_, irq_state);

        hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
        
        if (!delay_irq_set_)
        {
            delay_irq_set_ = true;
            irq_set_exclusive_handler(TIMER_IRQ(ALARM_NUM), timer_irq_handler);
            irq_set_enabled(TIMER_IRQ(ALARM_NUM), true);
        }

        irq_state = spin_lock_blocking(spinlock_delayed_);
        
        uint64_t target_time = timer_hw->timerawl + static_cast<uint64_t>(delay_ms) * 1000;

        for (auto& task : delayed_tasks_) 
        {
            if (!task.function) 
            {
                task.target_time = target_time;
                task.interval_ms = repeating ? delay_ms : 0;
                task.function = function;
                task.task_id = task_id;

                auto it = std::min_element(delayed_tasks_.begin(), delayed_tasks_.end(), [](const DelayedTask& a, const DelayedTask& b) 
                {
                    return a.function && (!b.function || a.target_time < b.target_time);
                });

                if (it != delayed_tasks_.end() && it->function) 
                {
                    timer_hw->alarm[ALARM_NUM] = static_cast<uint32_t>(it->target_time);
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

        for (auto& task : delayed_tasks_) 
        {
            if (task.task_id == task_id) 
            {
                task.function = nullptr;
                task.task_id = 0;
                found = true;
            }
        }

        if (!found || !delay_irq_set_)
        {
            spin_unlock(spinlock_delayed_, irq_state);
            return;
        }

        hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);

        auto it = std::min_element(delayed_tasks_.begin(), delayed_tasks_.end(), [](const DelayedTask& a, const DelayedTask& b) 
        {
            return a.function && (!b.function || a.target_time < b.target_time);
        });

        if (it != delayed_tasks_.end() && it->function) 
        {
            timer_hw->alarm[ALARM_NUM] = static_cast<uint32_t>(it->target_time);
        }
        
        spin_unlock(spinlock_delayed_, irq_state);
    }

    bool queue_task(const std::function<void()>& function)
    {
        uint32_t irq_state = spin_lock_blocking(spinlock_);
        for (auto& task : tasks_)
        {
            if (!task.function)
            {
                task.function = function;
                spin_unlock(spinlock_, irq_state);
                return true;
            }
        }
        spin_unlock(spinlock_, irq_state);
        return false;
    }

    void process_tasks()
    {
        uint32_t irq_state = spin_lock_blocking(spinlock_);
        for (auto& task : tasks_)
        {
            if (task.function)
            {
                auto function = task.function;
                task.function = nullptr;
                spin_unlock(spinlock_, irq_state);

                function();

                irq_state = spin_lock_blocking(spinlock_);
            }
            else
            {
                break; //No more tasks
            }
        }
        spin_unlock(spinlock_, irq_state);
    }

} // namespace Core1

namespace Core0 {

    static constexpr uint32_t CORE = 0;
    static constexpr uint32_t ALARM_NUM = CORE;

    static std::array<Task, MAX_TASKS> tasks_;
    static std::array<DelayedTask, MAX_DELAYED_TASKS> delayed_tasks_;

    static bool delay_irq_set_ = false;
    static uint32_t new_task_id_ = 1;

    static spin_lock_t* spinlock_ = spin_lock_instance(CORE);
    static spin_lock_t* spinlock_delayed_ = spin_lock_instance(CORE + 2);

    uint32_t get_new_task_id()
    {
        return new_task_id_++;
    }

    static void timer_irq_handler()
    {
        hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

        uint64_t now = get_time_64_us();
        uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);

        for (auto& task : delayed_tasks_) 
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

        auto it = std::min_element(delayed_tasks_.begin(), delayed_tasks_.end(), [](const DelayedTask& a, const DelayedTask& b) 
        {
            return a.function && (!b.function || a.target_time < b.target_time);
        });

        if (it != delayed_tasks_.end() && it->function) 
        {
            timer_hw->alarm[ALARM_NUM] = static_cast<uint32_t>(it->target_time);
        }

        spin_unlock(spinlock_delayed_, irq_state);
    }

    bool queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function)
    {
        uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);
        for (const auto& task : delayed_tasks_) 
        {
            if (task.task_id == task_id)
            {
                spin_unlock(spinlock_delayed_, irq_state);
                return false;
            }
        }
        spin_unlock(spinlock_delayed_, irq_state);

        hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
        
        if (!delay_irq_set_)
        {
            delay_irq_set_ = true;
            irq_set_exclusive_handler(TIMER_IRQ(ALARM_NUM), timer_irq_handler);
            irq_set_enabled(TIMER_IRQ(ALARM_NUM), true);
        }

        irq_state = spin_lock_blocking(spinlock_delayed_);
        
        uint64_t target_time = timer_hw->timerawl + static_cast<uint64_t>(delay_ms) * 1000;

        for (auto& task : delayed_tasks_) 
        {
            if (!task.function) 
            {
                task.target_time = target_time;
                task.interval_ms = repeating ? delay_ms : 0;
                task.function = function;
                task.task_id = task_id;

                auto it = std::min_element(delayed_tasks_.begin(), delayed_tasks_.end(), [](const DelayedTask& a, const DelayedTask& b) 
                {
                    return a.function && (!b.function || a.target_time < b.target_time);
                });

                if (it != delayed_tasks_.end() && it->function) 
                {
                    timer_hw->alarm[ALARM_NUM] = static_cast<uint32_t>(it->target_time);
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

        for (auto& task : delayed_tasks_) 
        {
            if (task.task_id == task_id) 
            {
                task.function = nullptr;
                task.task_id = 0;
                found = true;
            }
        }

        if (!found || !delay_irq_set_)
        {
            spin_unlock(spinlock_delayed_, irq_state);
            return;
        }

        hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);

        auto it = std::min_element(delayed_tasks_.begin(), delayed_tasks_.end(), [](const DelayedTask& a, const DelayedTask& b) 
        {
            return a.function && (!b.function || a.target_time < b.target_time);
        });

        if (it != delayed_tasks_.end() && it->function) 
        {
            timer_hw->alarm[ALARM_NUM] = static_cast<uint32_t>(it->target_time);
        }
        
        spin_unlock(spinlock_delayed_, irq_state);
    }

    bool queue_task(const std::function<void()>& function)
    {
        uint32_t irq_state = spin_lock_blocking(spinlock_);
        for (auto& task : tasks_)
        {
            if (!task.function)
            {
                task.function = function;
                spin_unlock(spinlock_, irq_state);
                return true;
            }
        }
        spin_unlock(spinlock_, irq_state);
        return false;
    }

    void process_tasks()
    {
        uint32_t irq_state = spin_lock_blocking(spinlock_);
        for (auto& task : tasks_)
        {
            if (task.function)
            {
                auto function = task.function;
                task.function = nullptr;
                spin_unlock(spinlock_, irq_state);

                function();

                irq_state = spin_lock_blocking(spinlock_);
            }
            else
            {
                break; //No more tasks
            }
        }
        spin_unlock(spinlock_, irq_state);
    }

} // namespace Core0

} // namespace TaskQueue