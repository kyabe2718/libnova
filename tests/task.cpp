#include <iostream>

#include <nova/task.hpp>
#include <nova/when_all.hpp>
#include <nova/sync_wait.hpp>
#include <nova/util/intrusive_queue.hpp>

struct PeriodicTaskExecutor {

    auto schedule() {
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

        void await_resume() const noexcept {}

        void execute() { coro.resume(); }

        explicit CoroTask(PeriodicTaskExecutor *executor) noexcept: executor(executor) {}

        nova::coro::coroutine_handle<> coro;
        CoroTask *next = nullptr;
        PeriodicTaskExecutor *executor;
    };

    nova::intrusive_queue<CoroTask, &CoroTask::next> task_list;
};

using nova::task;

int main() {

    PeriodicTaskExecutor executor;

    auto test = [&]() -> task<> {
        for (int i = 0; i < 4; ++i) {
            std::cout << "task_one " << i << std::endl;
            co_await executor.schedule();
        }
    };

    auto test2 = [&]() -> task<> {
        for (int i = 0; i < 8; ++i) {
            std::cout << "task_two " << i << std::endl;
            co_await executor.schedule();
        }
    };

    auto test3 = [&]() -> task<> {
        for (int i = 0; i < 7; ++i) {
            std::cout << "task_three " << i << std::endl;
            co_await executor.schedule();
        }
        co_await nova::when_all(test(), test2());
        co_return;
    };

    nova::sync_wait(test3(), [&] {
        while (!executor.empty()) {
            std::cout << "===================== next =========================" << std::endl;
            executor();
        }
    });
}
