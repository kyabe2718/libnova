#pragma once

#include <cstddef>
#include <utility>

namespace nova {

template<typename Item, Item *Item::*Next>
struct intrusive_queue {

    intrusive_queue() = default;
    explicit intrusive_queue(Item *head, Item *tail) noexcept
        : m_head(head), m_tail(tail) {}
    intrusive_queue(const intrusive_queue &) = delete;
    intrusive_queue &operator=(const intrusive_queue &) = delete;

    intrusive_queue(intrusive_queue &&other) noexcept
        : m_head(std::exchange(other.m_head, nullptr)),
          m_tail(std::exchange(other.m_tail, nullptr)) {}

    intrusive_queue &operator=(intrusive_queue &&other) noexcept {
        m_head = std::exchange(other.m_head, nullptr);
        m_tail = std::exchange(other.m_tail, nullptr);
        return *this;
    }

    [[nodiscard]] bool empty() const noexcept {
        return m_head == nullptr;
    }

    void push_back(Item *item) noexcept {
        if (m_tail)
            m_tail->*Next = item;
        m_tail = item;
        if (!m_head)
            m_head = item;
    }

    [[nodiscard]] Item *pop_front() noexcept {
        auto ret = m_head;
        if (m_head)
            m_head = m_head->*Next;
        if (!m_head)
            m_tail = nullptr;
        return ret;
    }

private:
    Item *m_head = nullptr;
    Item *m_tail = nullptr;
};

}// namespace nova