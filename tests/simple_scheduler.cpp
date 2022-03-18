#include <iostream>
#include <nova/simple_scheduler.hpp>
#include <nova/sync_wait.hpp>
#include <nova/task.hpp>
#include <nova/when_all.hpp>

#include <numeric>

using namespace std::chrono_literals;

std::atomic<int> t_cnt = 0;
thread_local int tid = t_cnt.fetch_add(1);
std::vector<int> counts;

int main() {
    std::cout << "start" << std::endl;
    nova::simple_scheduler rt;
    std::cout << &rt << std::endl;

    std::thread th{[&rt] {
        while (true) {
            std::this_thread::sleep_for(100ms);
            rt.print_workers(std::cout);
        }
    }};
    th.detach();

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&] {
            rt.run();
        });
    }
    counts.resize(threads.size(), 0);

    auto task = [&]() -> nova::task<> {
        co_await rt.schedule();
        std::this_thread::sleep_for(1ms);
        auto when_all = nova::when_all(std::vector<nova::task<>>{});
        auto subtask = [&rt = rt]() -> nova::task<> {
            co_await rt.schedule();
            std::this_thread::sleep_for(1ms);
            std::vector<nova::task<>> tasks;
            for (int i = 0; i < 100; ++i) {
                tasks.push_back([&rt]() -> nova::task<> {
                    co_await rt.schedule();
                    std::this_thread::sleep_for(1ms);
                    counts[tid]++;
                }());
            }
            co_await nova::when_all(std::move(tasks));
        };
        for (int i = 0; i < 10; ++i)
            when_all.add_task(subtask(), nova::launch::defer);
        std::cout << "wait_all" << std::endl;
        co_await when_all;
    };
    nova::sync_wait(task());

    rt.stop_request();
    for (auto &t: threads)
        t.join();

    for (auto &c: counts)
        std::cout << c << " ";
    std::cout << "\nsum: " << std::accumulate(counts.begin(), counts.end(), 0) << std::endl;
}