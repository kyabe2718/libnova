#pragma once

#include <atomic>

namespace nova {

template<typename Item, Item *Item::*Next>
struct atomic_intrusive_list {

    atomic_intrusive_list() noexcept : m_head(nullptr) {}
    atomic_intrusive_list(const atomic_intrusive_list &other) = delete;
    atomic_intrusive_list(atomic_intrusive_list &&other) = delete;
    atomic_intrusive_list &operator=(const atomic_intrusive_list &other) = delete;
    atomic_intrusive_list &operator=(atomic_intrusive_list &&other) = delete;

    using size_type = long;

    void push_front(Item *item) noexcept {
        auto current_head = m_head.load(std::memory_order_relaxed);
        do {
            item->*Next = static_cast<Item *>(current_head);
        } while (not m_head.compare_exchange_weak(
                current_head, item,
                std::memory_order_release,
                std::memory_order_acquire));
    }

    Item *pop_front() noexcept {
        auto current_head = m_head.load(std::memory_order_relaxed);
        Item *next = nullptr;
        do {
            if (!current_head) {
                return nullptr;
            } else {
                next = current_head->*Next;
            }
        } while (not m_head.compare_exchange_weak(
                current_head,
                next,
                std::memory_order_release,
                std::memory_order_acquire));
        return current_head;
    }

    //    void merge_front(intrusive_stack<Item, Next> list) {
    //        m_approx_size.fetch_add(list.m_size, std::memory_order_relaxed);
    //        auto current_head = m_head.load(std::memory_order_relaxed);
    //        do {
    //            list.m_tail->*Next = current_head;
    //        } while (not m_head.compare_exchange_weak(
    //                current_head,
    //                list.m_head,
    //                std::memory_order_release,
    //                std::memory_order_acquire));
    //    }
    //
    //    intrusive_stack<Item, Next> pop_all_forward() {
    //        m_approx_size.store(0, std::memory_order_relaxed);
    //
    //        auto old_head = m_head.load(std::memory_order_relaxed);
    //        if (old_head == nullptr)
    //            return {};
    //        old_head = m_head.exchange(nullptr, std::memory_order_acquire);
    //        return intrusive_stack<Item, Next>::make_forward(static_cast<Item *>(old_head));
    //    }
    //
    //    intrusive_stack<Item, Next> pop_all_reversed() {
    //        m_approx_size.store(0, std::memory_order_relaxed);
    //
    //        auto old_head = m_head.load(std::memory_order_relaxed);
    //        if (old_head == nullptr)
    //            return {};
    //        old_head = m_head.exchange(nullptr, std::memory_order_acquire);
    //        return intrusive_stack<Item, Next>::make_reversed(static_cast<Item *>(old_head));
    //    }

    [[nodiscard]] bool empty(std::memory_order order = std::memory_order_acquire) const noexcept {
        return m_head.load(order) == nullptr;
    }

private:
    explicit atomic_intrusive_list(void *head) noexcept
        : m_head(head) {}
    std::atomic<Item *> m_head;
};

}// namespace nova
