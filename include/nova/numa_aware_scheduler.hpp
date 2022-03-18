#pragma once

#include <nova/util/atomic_intrusive_list.hpp>
#include <nova/util/circular_view.hpp>
#include <nova/util/coroutine_base.hpp>
#include <nova/util/numa_info.hpp>
#include <nova/worker.hpp>

#include <memory>
#include <thread>
#include <vector>

#if NOVA_NUMA_AVAILABLE

namespace nova {
inline namespace scheduler {
struct numa_aware_scheduler {
    struct worker;
    using id_t = worker_base<worker>::id_t;

    task_base *try_steal(id_t cpu);
    void post(task_base *op);

    auto schedule() noexcept {
        struct [[nodiscard]] operation : task_base {
            explicit operation(numa_aware_scheduler *sched)
                : sched(sched), coro{} {}
            auto await_ready() const noexcept { return false; }
            void await_suspend(coro::coroutine_handle<> h) noexcept {
                coro = h;
                sched->post(this);
            }
            void await_resume() const noexcept {}
            void execute() override { coro.resume(); }

        private:
            numa_aware_scheduler *sched;
            coro::coroutine_handle<> coro;
        };
        return operation{this};
    }

    void start();
    void stop();

    explicit numa_aware_scheduler(std::size_t thread_num);

private:
    void run_worker(int cpu);

    numa_info info;
    std::vector<std::thread> thread_pool;
    std::vector<std::shared_ptr<worker>> workers;
    std::atomic<int> sleeping_worker_count = 0;
    atomic_intrusive_list<task_base, &task_base::next> global_task_queue;
};

}// namespace scheduler
}// namespace nova

#endif