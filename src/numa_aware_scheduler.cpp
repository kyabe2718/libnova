
#include <iostream>
#include <pthread.h>
#include <random>

#include <nova/numa_aware_scheduler.hpp>
#include <nova/util/circular_iterator.hpp>

#if NOVA_NUMA_AVAILABLE

namespace nova {
inline namespace scheduler {

struct numa_aware_scheduler::worker : worker_base<worker> {
    using base = worker_base<worker>;
    using base::this_thread_worker_id;
    friend numa_aware_scheduler;

    explicit worker(id_t id, numa_aware_scheduler &sched) : base(id), sched(&sched) {}

    void post(task_base *tb) {
        task_list.push_front(tb);
    }

    void try_sleep() {
        sched->sleeping_worker_count.fetch_add(1, std::memory_order_relaxed);
        base::try_sleep();
        sched->sleeping_worker_count.fetch_sub(1, std::memory_order_release);
    }

    auto execute_one() -> bool {
        if (!this_thread_worker_id) {
            throw std::runtime_error("simple_worker is executed on an unlinked thread.");
        }
        if (auto op = task_list.pop_front()) {
            op->execute();
            return true;
        }
        if (auto op = sched->try_steal(this->id)) {
            op->execute();
            return true;
        }
        return false;
    }

    atomic_intrusive_list<task_base, &task_base::next> task_list;
    numa_aware_scheduler *sched;
};

task_base *numa_aware_scheduler::try_steal(id_t cpu) {
    if (auto *op = global_task_queue.pop_front()) {
        return op;
    }

    auto get_cpus = [this](int node_id) {
        static thread_local std::random_device seed_gen;
        static thread_local std::mt19937 engine(seed_gen());
        auto cpus = info.node(node_id).cpus();
        std::shuffle(cpus.begin(), cpus.end(), engine);
        return cpus;
    };

    auto this_node = info.cpu_to_node(cpu);
    for (auto &node: info.near_nodes(this_node.id())) {
        for (auto c: get_cpus(node)) {
            if (auto &w = workers.at(c)) {
                if (auto op = w->task_list.pop_front()) {
                    return op;
                }
            }
        }
    }
    return nullptr;
}

void numa_aware_scheduler::post(task_base *op) {
    auto c = sleeping_worker_count.load(std::memory_order_acquire);
    if (c > 0) {
        using Iter = typename std::vector<int>::const_iterator;
        static thread_local std::vector<CircularIterator<Iter>> cpu_iters = [](auto &nodes) mutable {
            std::vector<CircularIterator<Iter>> ret;
            for (auto &node: nodes)
                ret.emplace_back(CircularIterator<Iter>(node.cpus().begin(), node.cpus().end(), node.cpus().begin()));
            return ret;
        }(info.nodes());

        auto this_node = info.cpu_to_node(worker::this_thread_worker_id.value_or(0));
        for (auto &node: info.near_nodes(this_node.id())) {
            auto &it = cpu_iters.at(node);
            for (auto i = 0u; i < info.node(node).cpus().size(); ++i) {
                auto cpu = *(it++);
                if (auto &w = workers.at(cpu)) {
                    if (w->try_wake_up([op](auto &&w) { w.post(op); })) {
                        return;
                    }
                }
            }
        }
    }

    if (c < 0)
        throw std::runtime_error("sleeping worker count must be positive. but current value is " + std::to_string(c));

    if (auto w = worker::this_thread_worker_id) {
        workers.at(*w)->post(op);
        workers.at(*w)->try_wake_up();
    } else {
        global_task_queue.push_front(op);
        for (auto &worker: workers)
            if (worker && worker->try_wake_up())
                return;
    }
}

void numa_aware_scheduler::start() {
    //    for (auto &n: info.nodes()) {
    //        std::cout << "node[" << n.id() << "]: ";
    //        for(auto& cpu : n.cpus())
    //            std::cout << cpu << " ";
    //        std::cout << std::endl;
    //    }

    using namespace std::string_literals;
    //    if (mallopt(M_TRIM_THRESHOLD, -1) == 0) {
    //        throw std::runtime_error("[M_TRIM_THRESHOLD] "s + strerror(errno));
    //    }
    //    if (mallopt(M_MMAP_THRESHOLD, 32 * 1024 * 1024) == 0) {
    //        throw std::runtime_error("[M_MMAP_THRESHOLD] "s + strerror(errno));
    //    }
    //    int fastbin_size = 80 * sizeof(size_t) / 4;
    //    if (mallopt(M_MXFAST, fastbin_size) == 0) {
    //        throw std::runtime_error("[M_MXFAST] "s + strerror(errno));
    //    }
    //    std::cout << "set fastbin size: " << fastbin_size << std::endl;

    for (int i = 0; i < int(thread_pool.size()); ++i) {
        thread_pool.at(i) = std::thread{[this, cpu = i] {
            this->run_worker(cpu);
        }};
    }
}

void numa_aware_scheduler::stop() {
    for (auto &w: workers) {
        if (w)
            w->stop_request();
    }
    for (auto &t: thread_pool)
        t.join();
}

void numa_aware_scheduler::run_worker(int cpu) {
    using namespace std::literals;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    auto self_th = pthread_self();
    if (auto e = pthread_setaffinity_np(self_th, sizeof(cpuset), &cpuset); e != 0) {
        throw std::runtime_error("[pthread_setaffinity_np] "s + strerror(e));
    }

    numa_set_localalloc();
    numa_set_strict(true);

    //        auto policy = SCHED_FIFO;
    //        sched_param param;
    //        param.sched_priority = sched_get_priority_max(policy);
    //        if (auto e = pthread_setschedparam(self_th, policy, &param); e != 0) {
    //            throw std::runtime_error("[pthread_setschedparam] "s + strerror(e));
    //        }

    workers.at(cpu) = std::make_shared<worker>(cpu, *this);
    workers.at(cpu)->run();
}

numa_aware_scheduler::numa_aware_scheduler(std::size_t thread_num)
    : thread_pool(thread_num), workers(numa_num_configured_cpus()) {}


}// namespace scheduler
}// namespace nova

#endif