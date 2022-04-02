#pragma once

#include <nova/type_traits.hpp>

#include <atomic>

namespace nova {

template<typename C>
struct abortable;

struct abort_exception {};

namespace detail {

template<typename Awaitable>
decltype(auto) apply_co_await(Awaitable &&awaitable) = delete;

template<non_member_co_await Awaitable>
decltype(auto) apply_co_await(Awaitable &&awaitable) {
    return operator co_await(std::forward<Awaitable>(awaitable));
}

template<member_co_await Awaitable>
decltype(auto) apply_co_await(Awaitable &&awaitable) {
    return std::forward<Awaitable>(awaitable).operator co_await();
}

template<typename E, typename P>
concept can_await_transform = requires(E &&e) {
    std::declval<P>().await_transform(std::forward<E>(e));
};

template<typename E, typename P>
concept can_yield_value = requires(E &&e) {
    std::declval<P>().yield_value(std::forward<E>(e));
};

struct abortable_promise_base {

    void abort() noexcept { flag.store(true, std::memory_order_release); }
    bool is_aborted() const noexcept { return flag.load(std::memory_order_acquire); }

private:
    std::atomic<bool> flag = false;
};

template<typename Awaiter>
struct aboartable_awaiter {

    auto await_ready() const {
        if (promise_base->is_aborted())
            throw abort_exception{};
        return awaiter.await_ready();
    }

    template<typename H>
    auto await_suspend(H &&h) { return awaiter.await_suspend(std::forward<H>(h)); }

    decltype(auto) await_resume() & {
        if (promise_base->is_aborted())
            throw abort_exception{};
        return awaiter.await_resume();
    }

    decltype(auto) await_resume() && {
        if (promise_base->is_aborted())
            throw abort_exception{};
        return std::move(awaiter).await_resume();
    }

    Awaiter awaiter;
    abortable_promise_base *promise_base;
};

template<typename Coro>
struct abortable_promise : Coro::promise_type, abortable_promise_base {
    using base = typename Coro::promise_type;

    using base::base;

    auto get_return_object() -> abortable<Coro>;

    void unhandled_exception() {
        auto ep = std::current_exception();
        try {
            std::rethrow_exception(ep);
        } catch (abort_exception &e) {
            return;
        } catch (...) {
            Coro::promise_type::unhandled_exception();
        }
    }

    template<typename E>
    auto await_transform(E &&e) {
        decltype(auto) awaiter = apply_co_await(std::forward<E>(e));
        return aboartable_awaiter<decltype(awaiter)>{std::forward<decltype(awaiter)>(awaiter), this};
    }

    template<can_await_transform<typename Coro::promise_type> E>
    auto await_transform(E &&e) {
        decltype(auto) awaiter = apply_co_await(Coro::promise_type::await_transform(std::forward<E>(e)));
        return aboartable_awaiter<decltype(awaiter)>{std::forward<decltype(awaiter)>(awaiter), this};
    }

    template<can_yield_value<typename Coro::promise_type> E>
    auto yield_value(E &&e) {
        decltype(auto) awaiter = Coro::promise_type::yield_value(std::forward<E>(e));
        return aboartable_awaiter<decltype(awaiter)>{std::forward<decltype(awaiter)>(awaiter), this};
    }
};
}// namespace detail

template<typename C>
struct abortable : public C {
    using promise_type = detail::abortable_promise<C>;

    explicit abortable(C &&c, detail::abortable_promise_base *base)
        : C(std::move(c)), base(base) {}

    void abort() { base->abort(); }

private:
    detail::abortable_promise_base *base;
};

template<typename Coro>
auto detail::abortable_promise<Coro>::get_return_object() -> abortable<Coro> {
    return abortable<Coro>(this->Coro::promise_type::get_return_object(), this);
}

}// namespace nova