#include <iostream>

#include <nova/abortable.hpp>
#include <nova/sync_wait.hpp>
#include <nova/task.hpp>
#include <nova/when_all.hpp>

#include <nova/util/intrusive_queue.hpp>

struct PeriodicTaskExecutor {

    auto operator co_await() {
        return CoroTask{this};
    }

    void operator()() {
        auto tl = std::move(task_list);
        while (auto task = tl.pop_front()) {
            task->execute();
        }
    }

    bool empty() const noexcept {
        return task_list.empty();
    }

private:
    struct CoroTask {

        bool await_ready() const noexcept { return false; }

        void await_suspend(nova::coro::coroutine_handle<> h) noexcept {
            coro = h;
            executor->task_list.push_back(this);
        }

        void await_resume() const {
            if (executor->is_stop) {
                throw std::runtime_error{"stop"};
            }
        }

        void execute() { coro.resume(); }

        explicit CoroTask(PeriodicTaskExecutor *executor) noexcept : executor(executor) {}

        nova::coro::coroutine_handle<> coro;
        CoroTask *next = nullptr;
        PeriodicTaskExecutor *executor;
    };

    nova::intrusive_queue<CoroTask, &CoroTask::next> task_list;

public:
    bool is_stop = false;
};

using nova::task;

int main() {
    PeriodicTaskExecutor executor;

    auto task = [](PeriodicTaskExecutor &e) -> nova::abortable<nova::task<void>> {
        for (int i = 0; i < 10; ++i) {
            try {
                co_await e;
            } catch (nova::abort_exception &e) {
                std::cout << "abort exception" << std::endl;
                throw;
            }
            std::cout << "hoge: " << i << std::endl;
        }
    };

    auto t = task(executor);

    nova::sync_wait(t, [&] {
        int i = 0;
        while (!executor.empty()) {
            if (i++ == 5)
                t.abort();
            std::cout << "===================== next =========================" << std::endl;
            executor();
        }
    });
}