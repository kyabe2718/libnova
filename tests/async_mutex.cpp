#include <nova/async_mutex.hpp>
#include <nova/simple_scheduler.hpp>
#include <nova/sync_wait.hpp>
#include <nova/task.hpp>

int main() {

    using namespace nova;

    nova::simple_scheduler sched(10);

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&] { sched.run(); });
    }

    int i = 0;
    async_mutex mtx;

    auto task = [&]() -> nova::task<> {
        co_await sched.schedule();
        {
            co_await mtx.async_lock();
            i++;
            mtx.unlock();
        }
        co_return;
    };


    nova::sync_wait(task());

    sched.stop_request();
    for (auto &th: threads)
        th.join();
}