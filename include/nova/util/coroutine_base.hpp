#pragma once

#include <nova/config.hpp>

#include <utility>

namespace nova {

template<typename P>
struct coroutine_base {
    using promise_type = P;

    coroutine_base() = delete;

    coroutine_base(const coroutine_base &) = delete;

    coroutine_base(coroutine_base &&other) noexcept
        : coro(std::exchange(other.coro, {})) {}

    coroutine_base &operator=(const coroutine_base &) = delete;

    coroutine_base &operator=(coroutine_base &&other) noexcept {
        coro = std::exchange(other.coro, {});
        return *this;
    }

    ~coroutine_base() {
        if (coro) {
            coro.destroy();
        }
    }

    auto &get_promise() { return coro.promise(); }
    const auto &get_promise() const { return coro.promise(); }

    [[nodiscard]] bool done() const { return coro.done(); }

    auto &get() noexcept { return coro; }
    const auto &get() const noexcept { return coro; }

protected:
    explicit coroutine_base(coro::coroutine_handle<promise_type> coro)
        : coro(coro) {}

    coro::coroutine_handle<promise_type> coro;
};

}// namespace nova
