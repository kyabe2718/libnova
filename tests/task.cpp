#include <iostream>
#include <nova/sync_wait.hpp>
#include <nova/task.hpp>
#include <nova/when_all.hpp>

struct Hoge {
    Hoge() { std::cout << __PRETTY_FUNCTION__ << std::endl; }
    ~Hoge() { std::cout << __PRETTY_FUNCTION__ << std::endl; }
    Hoge(const Hoge &) { std::cout << __PRETTY_FUNCTION__ << std::endl; }
    Hoge &operator=(const Hoge &) { std::cout << __PRETTY_FUNCTION__ << std::endl; }
    int n = 100;
};

int main() {

    nova::sync_wait([]() -> nova::task<> {
        auto tasks = nova::when_all(std::vector<nova::task<>>{});
        auto t = [h = Hoge{}]() -> nova::task<> {
            std::cout << __LINE__ << std::endl;
            co_return;
        };
        std::cout << __LINE__ << std::endl;
        tasks.add_task(t());
        std::cout << __LINE__ << std::endl;
        co_await tasks;
    }());
}
