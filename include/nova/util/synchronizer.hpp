#pragma once

#include <condition_variable>
#include <mutex>

#include <iostream>

#include <nova/config.hpp>

namespace nova {

struct cv_synchronizer {
    void wait() {
        std::unique_lock lk(mtx);
        cv.wait(lk, [this] { return st; });
    }

    void notify() {
        std::unique_lock lk(mtx);
        st = true;
        cv.notify_one();
    }

    void notify_all() {
        std::unique_lock lk(mtx);
        st = true;
        cv.notify_all();
    }

    auto state() const noexcept { return st; }

private:
    std::mutex mtx;
    std::condition_variable cv;
    bool st = false;
};

#if NOVA_ATOMIC_WAIT_NOTIFY_AVAILABLE
struct futex_synchronizer {
    void wait() {
        while (!flag.test(std::memory_order_acquire)) {
            flag.wait(false, std::memory_order_acquire);
        }
    }

    void notify() {
        flag.test_and_set(std::memory_order_release);
        flag.notify_one();
    }

    void notify_all() {
        flag.test_and_set(std::memory_order_release);
        flag.notify_all();
    }

    bool state(std::memory_order m = std::memory_order_acquire) const noexcept {
        return flag.test(m);
    }

private:
    std::atomic_flag flag = {};
};
#endif

#if NOVA_ATOMIC_WAIT_NOTIFY_AVAILABLE
using synchronizer = futex_synchronizer;
#else
using synchronizer = cv_synchronizer;
#endif

template<typename T>
struct cv_sync_atomic : std::atomic<T> {

    using std::atomic<T>::atomic;

    void wait(T old, std::memory_order m = std::memory_order_seq_cst) const noexcept {
        std::unique_lock lk(mtx);
        if (this->load(m) == old) {
            cv.wait(lk);
        }
    }

    void notify_one() noexcept {
        std::unique_lock lk(mtx);
        cv.notify_one();
    }

    void notify_all() noexcept {
        std::unique_lock lk(mtx);
        cv.notify_all();
    }

private:
    mutable std::mutex mtx;
    mutable std::condition_variable cv;
};

#if NOVA_ATOMIC_WAIT_NOTIFY_AVAILABLE
template<typename T>
using sync_atomic = std::atomic<T>;
#else
template<typename T>
using sync_atomic = cv_sync_atomic<T>;
#endif

}// namespace nova