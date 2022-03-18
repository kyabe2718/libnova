#pragma once

#include <nova/util/coroutine_base.hpp>

namespace nova {

template<typename YieldType>
struct generator;

namespace detail {
template<typename Y>
struct promise_yield_adopter {
    template<typename T>
    auto yield_value(T &&v) {
        value = std::forward<T>(v);
        return std::suspend_always{};
    }

    Y value;
};

template<>
struct promise_yield_adopter<void> {
    struct dummy {};
    auto yield_value(dummy = {}) {
        return std::suspend_always{};
    }
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

namespace detail {
template<typename Y>
struct current_value_adopter {
    auto current_value() & -> auto & {
        return this->get_promise().value;
    }

    auto current_value() const & -> const auto & {
        return this->get_promise().value;
    }

    auto current_value() && -> auto && {
        return std::move(this->get_promise().value);
    }
};

template<>
struct current_value_adopter<void> {};
}// namespace detail

template<typename Y>
struct [[nodiscard]] generator : detail::current_value_adopter<Y>, coroutine_base<generator_promise<Y>> {

    using promise_type = generator_promise<Y>;

    friend promise_type;

    using coroutine_base<promise_type>::coroutine_base;

    bool move_next() {
        if (!this->done()) {
            this->coro.resume();
            return !this->done();
        } else {
            return false;
        }
    }
};
}// namespace nova