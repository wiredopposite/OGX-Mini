#include "TaskQueue/TaskQueue.h"

TaskQueue::TaskQueue(CoreNum core_num) 
{   
    alarm_num_ = (core_num == CoreNum::Core0) ? 0 : 1;
    alarm_num_ += (OGXM_BOARD == PI_PICOW) ? 1 : 0; //BTStack uses alarm 0

    hw_set_bits(&timer_hw->inte, 1u << alarm_num_);

    irq_set_exclusive_handler(
        TIMER_IRQ(alarm_num_), 
        (core_num == CoreNum::Core0) ? timer_irq_wrapper_c0 : timer_irq_wrapper_c1);

    irq_set_enabled(TIMER_IRQ(alarm_num_), true);
}

uint32_t TaskQueue::get_new_task_id()
{
    return new_task_id_++;
}

bool TaskQueue::queue_delayed_task(uint32_t task_id, uint32_t delay_ms, bool repeating, const std::function<void()>& function)
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

void TaskQueue::cancel_delayed_task(uint32_t task_id)
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

bool TaskQueue::queue_task(const std::function<void()>& function)
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

void TaskQueue::process_tasks()
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

uint64_t TaskQueue::get_time_64_us()
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

void TaskQueue::timer_irq_handler()
{
    hw_clear_bits(&timer_hw->intr, 1u << alarm_num_);

    uint64_t now = get_time_64_us();
    uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);
    if (suspended_)
    {
        spin_unlock(spinlock_delayed_, irq_state);
        return;
    }

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

void TaskQueue::suspend_delayed_tasks()
{
    get_core0().suspend_delayed();
#if (OGXM_BOARD != PI_PICOW) && (OGXM_BOARD != PI_PICO2W)
    get_core1().suspend_delayed();
#endif
}

void TaskQueue::resume_delayed_tasks()
{
    get_core0().resume_delayed();
#if (OGXM_BOARD != PI_PICOW) && (OGXM_BOARD != PI_PICO2W)
    get_core1().resume_delayed();
#endif
}

void TaskQueue::suspend_delayed()
{
    uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);
    if (suspended_)
    {
        spin_unlock(spinlock_delayed_, irq_state);
        return;
    }
    hw_clear_bits(&timer_hw->intr, 1u << alarm_num_);
    suspended_time_ = get_time_64_us();
    suspended_ = true;
    spin_unlock(spinlock_delayed_, irq_state);
}

void TaskQueue::resume_delayed()
{
    uint32_t irq_state = spin_lock_blocking(spinlock_delayed_);
    if (!suspended_)
    {
        spin_unlock(spinlock_delayed_, irq_state);
        return;
    }

    uint64_t now = get_time_64_us();
    uint64_t elapsed_time = now - suspended_time_;

    for (auto& task : task_queue_delayed_) 
    {
        if (task.function) 
        {
            task.target_time = std::max(task.target_time + elapsed_time, now + 10);
        }
    }
    int64_t next_target_time = get_next_target_time_unsafe(task_queue_delayed_);
    if (next_target_time >= 0) 
    {
        timer_hw->alarm[alarm_num_] = static_cast<uint32_t>(next_target_time);
    }
    suspended_ = false;
    spin_unlock(spinlock_delayed_, irq_state);
}