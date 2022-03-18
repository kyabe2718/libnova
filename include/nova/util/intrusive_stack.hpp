#pragma once

#include <cstddef>

namespace nova {

template<typename Item, Item *Item::*Next>
struct intrusive_stack {

    intrusive_stack() = default;
    explicit intrusive_stack(Item *head) noexcept : m_head(head) {}
    intrusive_stack(const intrusive_stack &) = delete;
    intrusive_stack &operator=(const intrusive_stack &) = delete;

    intrusive_stack(intrusive_stack &&other) noexcept
        : m_head(std::exchange(other.m_head, nullptr)) {}

    intrusive_stack &operator=(intrusive_stack &&other) noexcept {
        m_head = std::exchange(other.m_head, nullptr);
        return *this;
    }

    [[nodiscard]] bool empty() const noexcept {
        return m_head == nullptr;
    }

    void push_front(Item *item) noexcept {
        item->*Next = m_head;
        m_head = item;
    }

    [[nodiscard]] Item *pop_front() noexcept {
        auto ret = m_head;
        if (!empty())
            m_head = m_head->*Next;
        return ret;
    }

    static intrusive_stack make_forward(Item *head) {
        return intrusive_stack(head);
    }

private:
    Item *m_head = nullptr;
};

}// namespace nova