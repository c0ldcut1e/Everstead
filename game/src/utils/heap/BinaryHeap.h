#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

template<typename T, typename Compare = std::less<T>>
class BinaryHeap
{
public:
    explicit BinaryHeap(const Compare &compare = Compare())
    {
        m_data    = {};
        m_compare = compare;
    }

    bool Empty() const
    {
        return m_data.empty();
    }

    size_t Size() const
    {
        return m_data.size();
    }

    void Clear()
    {
        m_data.clear();
    }

    const T &Top() const
    {
        return m_data.front();
    }

    void Push(const T &value)
    {
        m_data.push_back(value);
        std::push_heap(m_data.begin(), m_data.end(), m_compare);
    }

    void Push(T &&value)
    {
        m_data.push_back(std::move(value));
        std::push_heap(m_data.begin(), m_data.end(), m_compare);
    }

    void Pop()
    {
        std::pop_heap(m_data.begin(), m_data.end(), m_compare);
        m_data.pop_back();
    }

private:
    std::vector<T> m_data;
    Compare m_compare;
};
