#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>


template <typename T, std::size_t Capacity>
class SPSCQueue {
    static_assert(
        Capacity >= 2,
        "Capacity must be at least 2"
    );

    static_assert(
        (Capacity & (Capacity - 1)) == 0,
        "Capacity must be a power of two"
    );

    static_assert(
        std::atomic<std::size_t>::is_always_lock_free,
        "std::atomic<size_t> must be lock-free"
    );

    static_assert(
        std::is_nothrow_destructible_v<T>,
        "T must be nothrow destructible"
    );

private:
    static constexpr std::size_t Mask = Capacity - 1;

    // Raw storage.
    //
    // T is NOT constructed here.
    // The object lifetime begins when construct_at() is called.
    struct Node {
        alignas(alignof(T))
        std::byte storage[sizeof(T)];
    };

    Node buffer_[Capacity];

    // --------------------------------------------------------
    // Producer-owned state
    // --------------------------------------------------------
    //
    // Producer writes tail_.
    // Producer reads head_ only when its cached copy says
    // that the queue may be full.
    //
    // Keeping producer-owned and consumer-owned state on
    // separate cache lines reduces false sharing.
    alignas(std::hardware_destructive_interference_size)
    std::atomic<std::size_t> tail_{0};

    std::size_t head_cached_{0};

    // --------------------------------------------------------
    // Consumer-owned state
    // --------------------------------------------------------
    alignas(std::hardware_destructive_interference_size)
    std::atomic<std::size_t> head_{0};

    std::size_t tail_cached_{0};

    static T* object_ptr(Node& node) noexcept {
        return reinterpret_cast<T*>(node.storage);
    }

    static const T* object_ptr(const Node& node) noexcept {
        return reinterpret_cast<const T*>(node.storage);
    }

public:
    SPSCQueue() noexcept = default;

    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;

    ~SPSCQueue() noexcept {
        std::size_t head = head_.load(std::memory_order_relaxed);

        const std::size_t tail = tail_.load(std::memory_order_relaxed);

        while (head != tail) {
            std::destroy_at(object_ptr(buffer_[head & Mask]));

            ++head;
        }
    }

    template <typename... Args>
    bool try_emplace(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
    {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail - head_cached_ == Capacity) {

            head_cached_ = head_.load(std::memory_order_acquire);

            if (tail - head_cached_ == Capacity) {
                return false;
            }
        }

        std::construct_at(
            object_ptr(buffer_[tail & Mask]),
            std::forward<Args>(args)...
        );

        tail_.store(tail + 1, std::memory_order_release);

        return true;
    }

    bool try_push(const T& value)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        return try_emplace(value);
    }

    bool try_push(T&& value)
        noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        return try_emplace(std::move(value));
    }

    bool try_pop(T& value)
        noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_destructible_v<T>) 
    {
        static_assert(
            std::is_nothrow_move_assignable_v<T>,
            "T must be nothrow move assignable");

        static_assert(
            std::is_nothrow_destructible_v<T>,
            "T must be nothrow destructible");        
        const std::size_t head = head_.load(std::memory_order_relaxed);

        if (head == tail_cached_) {

            tail_cached_ = tail_.load(std::memory_order_acquire);

            if (head == tail_cached_) {
                return false;
            }
        }

        T* item = object_ptr(buffer_[head & Mask]);

        value = std::move(*item);

        std::destroy_at(item);

        head_.store(head + 1, std::memory_order_release);

        return true;
    }

    [[nodiscard]]
    bool empty() const noexcept {
        return
            head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }

    [[nodiscard]]
    std::size_t size() const noexcept {
        return tail_.load(std::memory_order_relaxed) - head_.load(std::memory_order_relaxed);;
    }
};