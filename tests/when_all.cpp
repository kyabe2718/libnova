#include <iostream>
#include <nova/sync_wait.hpp>
#include <nova/task.hpp>
#include <nova/when_all.hpp>

struct Hoge {
    int n;
};

int main() {


    auto task2 = []() -> nova::task<std::vector<Hoge>> {
        auto task1 = [](int n) -> nova::task<Hoge> {
            Hoge h{n};
            std::cout << "task1: " << n << std::endl;
            co_return h;
        };

        co_await nova::when_all(task1(1), task1(2));

        std::vector<nova::task<Hoge>> tasks{};
        tasks.push_back(task1(3));
        tasks.push_back(task1(4));
        auto a = nova::when_all(std::move(tasks));
        a.add_task(task1(5), nova::launch::immediate);
        a.add_task(task1(6), nova::launch::immediate);
        a.add_task(task1(7), nova::launch::defer);
        std::cout << "co_await a" << std::endl;
        co_return co_await a;
    };

    nova::sync_wait(task2());
}
