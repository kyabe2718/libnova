#pragma once

#include <nova/task.hpp>
#include <nova/type_traits.hpp>
#include <nova/util/coroutine_base.hpp>
#include <nova/util/return_value_or_void.hpp>
#include <nova/util/synchronizer.hpp>

#include <iostream>
#include <utility>

namespace nova {

template<typename T, typename Sync = synchronizer>
struct sync_wait_task;

template<typename T, typename Sync>
struct sync_wait_promise : return_value_or_void<T>, Sync {

    //    auto start() {
    //         coro::coroutine_handle<sync_wait_promise>::from_promise(*this).resume();
    //    }

    auto initial_suspend() -> coro::suspend_always { return {}; }

    auto final_suspend() noexcept {
        struct finish_notifier {
            auto await_ready() const noexcept { return false; }
            auto await_suspend(coro::coroutine_handle<sync_wait_promise> handle) const noexcept {
                return handle.promise().notify();
            }
            auto await_resume() const noexcept {}
        };
        return finish_notifier{};
    }

    auto get_return_object() -> sync_wait_task<T, Sync> {
        return sync_wait_task<T, Sync>{coro::coroutine_handle<sync_wait_promise>::from_promise(*this)};
    }
};

template<typename T, typename Sync>
struct sync_wait_task : coroutine_base<sync_wait_promise<T, Sync>> {
    using promise_type = sync_wait_promise<T, Sync>;
    friend promise_type;
    using coroutine_base<promise_type>::coroutine_base;

    auto start() {
        this->coro.resume();
    }

    auto wait() {
        this->get_promise().wait();
    }
};

namespace detail {
template<typename Awaitable, typename AwaiterResultType = typename awaitable_traits<Awaitable>::awaiter_result_t>
auto make_sync_wait_task(Awaitable &&awaitable) -> sync_wait_task<AwaiterResultType> {
    if constexpr (std::is_void_v<AwaiterResultType>) {
        co_await std::forward<decltype(awaitable)>(awaitable);
    } else {
        co_return co_await std::forward<decltype(awaitable)>(awaitable);
    }
}
}// namespace detail

template<typename Awaitable, typename F>
auto sync_wait(Awaitable &&awaitable, F &&f) -> decltype(auto) {

    auto wait_task = detail::make_sync_wait_task(std::forward<Awaitable>(awaitable));

    wait_task.start();
    f();
    wait_task.wait();

    if constexpr (std::is_rvalue_reference_v<Awaitable &&>) {
        return std::move(wait_task.get_promise()).result();
    } else {
        return wait_task.get_promise().result();
    }
}

template<typename Awaitable>
auto sync_wait(Awaitable &&awaitable) -> decltype(auto) {
    return sync_wait(std::forward<Awaitable>(awaitable), [] {});
}
}// namespace nova