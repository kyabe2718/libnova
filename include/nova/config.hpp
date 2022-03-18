#pragma once

#if __has_include(<coroutine>)
#include <coroutine>
namespace nova {
namespace coro = std;
}
#elif __has_include(<experimental/coroutine>)

#include <experimental/coroutine>

namespace nova {
namespace coro = std::experimental::coroutines_v1;
}
#endif

#include <utility>
#include <iostream>

namespace nova {

struct void_t {};
inline constexpr void_t void_v{};

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
            if(!coro.done())
                std::cout << "!coro.done()" << std::endl;
            coro.destroy();
        }
    }

    auto &get_promise() { return coro.promise(); }
    const auto &get_promise() const { return coro.promise(); }

    bool done() const { return coro.done(); }

protected:
    explicit coroutine_base(coro::coroutine_handle<promise_type> coro)
        : coro(coro) {}

    coro::coroutine_handle<promise_type> coro;
};

}// namespace nova