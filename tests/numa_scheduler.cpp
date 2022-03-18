#include <iostream>
#include <nova/numa_aware_scheduler.hpp>

int main() {
    // hoge
    nova::numa_info info;
    for (auto &node: info.nodes()) {
        std::cout << node.id() << ": ";
        std::cout << "[";
        for (auto &cpu: node.cpus())
            std::cout << cpu << " ";
        std::cout << "]" << std::endl;
    }

    for (auto &node: info.nodes()) {
        for (auto &distance: node.distances()) {
            std::cout << distance << " ";
        }
        std::cout << std::endl;
    }

    for (auto &node: info.nodes()) {
        std::cout << node.id() << ": ";
        for (auto &n: info.near_nodes(node.id())) {
            std::cout << n << " ";
        }
        std::cout << std::endl;
    }


    //    nova::numa_aware_scheduler sched{100};
    //    std::cout << "unchi" << std::endl;
}