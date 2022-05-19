#pragma once

#include <atomic>

#include <nova/config.hpp>

namespace nova {

struct async_mutex {

    async_mutex() = default;

    auto async_lock() { return lock_operation{this}; }

    void unlock();

    struct [[nodiscard]] lock_operation {

        explicit lock_operation(async_mutex *mutex) : mutex(mutex), next(nullptr) {}

        auto await_ready() const noexcept -> bool { return false; }

        // no need to wait lock -> false
        // need to wait lock -> true
        auto await_suspend(coro::coroutine_handle<> awaiter) noexcept -> bool;

        auto await_resume() const noexcept -> void {}

    private:
        async_mutex *mutex;
        lock_operation *next;
    };

private:
    inline static constexpr std::uintptr_t not_locked = 0;
    inline static constexpr std::uintptr_t locked_no_waiters = 1;

    std::atomic<std::uintptr_t> state = not_locked;
};

auto async_mutex::lock_operation::await_suspend(coro::coroutine_handle<> awaiter) noexcept -> bool {
    auto old_state = mutex->state.load(std::memory_order_acquire);
    while (true) {
        if (old_state == not_locked) {
            if (mutex->state.compare_exchange_weak(
                        old_state, locked_no_waiters,
                        std::memory_order_acquire, std::memory_order_relaxed)) {
                return false;// acquired lock, not suspend now.
            }
        } else {
            next = reinterpret_cast<async_mutex::lock_operation *>(old_state);
            if (mutex->state.compare_exchange_weak(
                        old_state, reinterpret_cast<std::uintptr_t>(this),
                        std::memory_order_release, std::memory_order_relaxed)) {
                return true;// queue to waiters list, suspend now.
            }
        }
    }
}

}// namespace nova