#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>
#include <thread>
#include <type_traits>
#include <utility>

template <typename T, size_t SIZE>
class RingBuffer {
public:
    RingBuffer() noexcept : m_size(0), m_head(0), m_tail(0) {}
    ~RingBuffer() {
        // I am lazy to write a faster implementation.
        while (!IsEmpty()) {
            Pop();
        }
    }

    RingBuffer(const RingBuffer &) = delete;
    RingBuffer &operator=(const RingBuffer &) = delete;

    RingBuffer(RingBuffer &&) = delete;
    RingBuffer &operator=(RingBuffer &&) = delete;

    bool IsEmpty() const noexcept {
        return (m_size.load(std::memory_order_relaxed) == 0);
    }

    bool IsFull() const noexcept {
        return (m_head.load(std::memory_order_relaxed) + SIZE <=
                m_tail.load(std::memory_order_relaxed));
    }

    /// @brief Push an element to the end of current ring buffer.
    /// @note This method may block current thread if this ring buffer is
    /// already full.
    void Push(const T &value) noexcept(
        std::is_nothrow_copy_constructible<T>::value) {
        uint64_t index = m_tail.fetch_add(1, std::memory_order_relaxed);
        while (index - m_head.load(std::memory_order_relaxed) >= SIZE)
            std::this_thread::yield();

        new (static_cast<void *>(m_data + (index % SIZE))) T(value);
        m_size.fetch_add(1, std::memory_order_acquire);
        m_available[index % SIZE].store(true, std::memory_order_relaxed);
    }

    /// @brief Push an element to the end of current ring buffer.
    /// @note This method may block current thread if this ring buffer is
    /// already full.
    void
    Push(T &&value) noexcept(std::is_nothrow_move_constructible<T>::value) {
        uint64_t index = m_tail.fetch_add(1, std::memory_order_relaxed);
        while (index - m_head.load(std::memory_order_relaxed) >= SIZE)
            std::this_thread::yield();

        new (static_cast<void *>(m_data + (index % SIZE))) T(std::move(value));
        m_size.fetch_add(1, std::memory_order_acquire);
        m_available[index % SIZE].store(true, std::memory_order_relaxed);
    }

    /// @brief Pop and get the element at the end of this ring buffer.
    /// @note This method may block current thread if this is an empty ring
    /// buffer.
    T Pop() noexcept(std::is_nothrow_move_constructible<T>::value) {
        uint64_t index = m_head.load(std::memory_order_relaxed);
        while (!m_available[index % SIZE].load(std::memory_order_relaxed))
            std::this_thread::yield();

        m_available[index % SIZE].store(false, std::memory_order_relaxed);
        m_size.fetch_sub(1, std::memory_order_release);
        T ret(std::move(m_data[index % SIZE]));
        m_head.fetch_add(1, std::memory_order_relaxed);
        return ret;
    }

private:
    std::atomic_size_t   m_size;
    std::atomic_uint64_t m_head;
    std::atomic_uint64_t m_tail;
    T                    m_data[SIZE];
    std::atomic_bool     m_available[SIZE]{};
};

#endif // RINGBUFFER_H
