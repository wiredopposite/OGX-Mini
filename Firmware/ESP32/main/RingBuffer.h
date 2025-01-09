#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <cstdint>
#include <atomic>
#include <array>

template<typename Type, size_t SIZE>
class RingBuffer
{
public:
    RingBuffer() : head_(0), tail_(0) {}

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    RingBuffer(RingBuffer&&) = default;
    RingBuffer& operator=(RingBuffer&&) = default;

    bool push(const Type& item)
    {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next_head = (head + 1) % SIZE;

        if (next_head == tail_.load(std::memory_order_acquire))
        {
            tail_.store((tail_.load(std::memory_order_relaxed) + 1) % SIZE, std::memory_order_release);
        }

        buffer_[head] = item;
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    bool pop(Type& item)
    {
        size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire))
        {
            return false;
        }

        item = buffer_[tail];
        tail_.store((tail + 1) % SIZE, std::memory_order_release);
        return true;
    }

private:
    std::array<Type, SIZE> buffer_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};

#endif // _RING_BUFFER_H_