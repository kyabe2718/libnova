#pragma once

#include <nova/config.hpp>
#include <nova/util/atomic_intrusive_list.hpp>
#include <nova/worker.hpp>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace nova {

inline namespace scheduler {

struct simple_scheduler {

    struct worker_t;
    using id_t = worker_base<worker_t>::id_t;

    explicit simple_scheduler(std::size_t worker_buffer_size = std::thread::hardware_concurrency())
        : workers(worker_buffer_size) {}

    task_base *try_steal(id_t stealer);

    void delegate(task_base *op, [[maybe_unused]] std::optional<id_t> source_worker);

    void post(task_base *op);

    void run();

    auto schedule() noexcept {
        struct [[nodiscard]] operation : task_base {
            explicit operation(simple_scheduler *sched) : sched(sched) {}
            auto await_ready() const noexcept { return false; }
            void await_suspend(coro::coroutine_handle<> h) noexcept {
                coro = h;
                sched->post(this);
            }
            void await_resume() const noexcept {}
            void execute() override {
                coro.resume();
            }

        private:
            simple_scheduler *sched;
            coro::coroutine_handle<> coro;
        };
        return operation{this};
    }

    void stop_request();

private:
    std::vector<std::shared_ptr<worker_t>> workers;
    std::atomic<std::size_t> worker_count = 0;
    atomic_intrusive_list<task_base, &task_base::next> global_task_queue;
};

}// namespace scheduler
}// namespace nova