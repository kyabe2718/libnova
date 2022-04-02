#pragma once

#include <nova/util/coroutine_base.hpp>
#include <nova/util/return_value_or_void.hpp>

namespace nova {

template<typename YieldType>
struct generator;

namespace detail {
template<typename Y>
struct promise_yield_adopter {
    template<typename T>
    auto yield_value(T &&v) {
        *reinterpret_cast<Y *>(data) = std::forward<T>(v);
        return coro::suspend_always{};
    }

    decltype(auto) get_value() { return *reinterpret_cast<Y *>(data); }
    decltype(auto) get_value() const { return *reinterpret_cast<const Y *>(data); }

private:
    alignas(Y) std::byte data[sizeof(Y)];
};

template<>
struct promise_yield_adopter<void> {
    struct dummy {};
    auto yield_value(dummy = {}) { return coro::suspend_always{}; }

    auto get_value() const {}
};
}// namespace detail

template<typename Y>
struct generator_promise : detail::promise_yield_adopter<Y>, return_value_or_void<void> {
    auto initial_suspend() -> coro::suspend_always { return {}; }

    auto final_suspend() noexcept -> coro::suspend_always { return {}; }

    auto get_return_object() -> generator<Y> {
        return generator<Y>{coro::coroutine_handle<generator_promise<Y>>::from_promise(*this)};
    }
};

template<typename Y>
struct [[nodiscard]] generator : coroutine_base<generator_promise<Y>> {

    using promise_type = generator_promise<Y>;

    friend promise_type;

    using coroutine_base<promise_type>::coroutine_base;

    void next() {
        if (!this->done()) {
            this->coro.resume();
        }
    }

    decltype(auto) value() { return this->get_promise().get_value(); }
    decltype(auto) value() const { return this->get_promise().get_value(); }

    //    decltype(auto) operator()() { return next(), value(); }
    //    decltype(auto) operator()() const { return next(), value(); }
};
}// namespace nova
