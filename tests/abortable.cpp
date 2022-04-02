#include <iostream>

#include <nova/abortable.hpp>
#include <nova/generator.hpp>

struct Hoge {
    Hoge() {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }

    ~Hoge() {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }
};

int main() {

    auto t = []() -> nova::abortable<nova::generator<int>> {
        Hoge hoge;
        for (int i = 0; i < 10; ++i) {
            std::cout << "co_yield " << i << std::endl;
            co_yield i;
        }
    }();

    for (int i = 0; not t.done(); ++i, t.next()) {
        if (i == 5)
            t.abort();
        std::cout << t.value() << " " << t.done() << std::endl;
    }
}