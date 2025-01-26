/*
 * SPDX-FileCopyrightText: 2015-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef NAIVEVECTOR_H
#define NAIVEVECTOR_H

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <new>
#include <stdexcept>
#include <type_traits>

namespace libime {

template <typename T>
struct naivevector {
    static_assert(std::is_trivially_destructible<T>::value &&
                      std::is_standard_layout<T>::value,
                  "this class should only use with trivially copyable class, "
                  "but well, we only "
                  "care about fundamental type");

    using value_type = T;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = value_type *;
    using const_iterator = const value_type *;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    naivevector() noexcept : m_start(nullptr), m_end(nullptr), m_cap(nullptr) {}

    ~naivevector() noexcept { std::free(m_start); }
    naivevector(const naivevector &other) : naivevector() {
        reserve(other.size());
        for (auto value : other) {
            push_back(value);
        }
    }
    naivevector(naivevector &&other) noexcept : naivevector() { swap(other); }

    naivevector &operator=(naivevector other) {
        swap(other);
        return *this;
    }

    void swap(naivevector &__other) noexcept {
        using std::swap;
        swap(m_start, __other.m_start);
        swap(m_end, __other.m_end);
        swap(m_cap, __other.m_cap);
    }

    // Iterators.
    iterator begin() noexcept { return iterator(data()); }

    const_iterator begin() const noexcept { return const_iterator(data()); }

    iterator end() noexcept { return iterator(m_end); }

    const_iterator end() const noexcept { return const_iterator(m_end); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    const_iterator cbegin() const noexcept { return const_iterator(data()); }

    const_iterator cend() const noexcept { return const_iterator(m_end); }

    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(begin());
    }

    // Capacity.
    size_type size() const noexcept { return size_type(m_end - m_start); }

    constexpr size_type max_size() const noexcept {
        return size_type(-1) / sizeof(value_type);
    }

    void clear() { resize(0); }

    void resize(size_type new_size) {
        if (new_size > size()) {
            auto old_size = size();
            auto cap = capacity();
            while (new_size > cap) {
                cap = cap ? (2 * cap) : 32;
            }
            reserve(cap);

            m_end = m_start + new_size;
            if constexpr (!std::is_trivial_v<value_type>) {
                for (auto *p = m_start + old_size; p != m_end; p++) {
                    new (p) value_type();
                }
            } else {
                std::memset(m_start + old_size, 0,
                            sizeof(value_type) * (new_size - old_size));
            }
        } else {
            m_end = m_start + new_size;
        }
    }

    void shrink_to_fit() {
        if (m_cap > m_end) {
            _realloc_array(size_type(reinterpret_cast<char *>(m_end) -
                                     reinterpret_cast<char *>(m_start)));
        }
    }

    void reserve(size_type new_size) {
        if (new_size > max_size()) {
            throw std::length_error("larger than max_size");
        }

        if (new_size > capacity()) {
            _realloc_array(new_size * sizeof(value_type));
        }
    }

    size_type capacity() const noexcept { return size_type(m_cap - m_start); }

    bool empty() const noexcept { return size() == 0; }

    // Element access.
    reference operator[](size_type __n) noexcept { return m_start[__n]; }

    const_reference operator[](size_type __n) const noexcept {
        return m_start[__n];
    }

    reference at(size_type __n) {
        if (__n >= size()) {
            throw std::out_of_range("out_of_range");
        }
        return m_start[__n];
    }

    const_reference at(size_type __n) const {
        if (__n >= size()) {
            throw std::out_of_range("out_of_range");
        }
        return m_start[__n];
    }

    reference front() noexcept { return *begin(); }

    const_reference front() const noexcept { return *cbegin(); }

    reference back() noexcept { return size() ? *(end() - 1) : *end(); }

    const_reference back() const noexcept {
        return size() ? *(cend() - 1) : *cend();
    }

    pointer data() noexcept { return m_start; }

    const_pointer data() const noexcept { return m_start; }

    void push_back(const value_type &__x) { emplace_back(__x); }

    void push_back(value_type &&__x) { emplace_back(std::move(__x)); }

    template <typename... _Args>
    void emplace_back(_Args &&...__args) {
        if (m_end == m_cap) {
            reserve(capacity() ? (2 * capacity()) : 32);
        }
        new (m_end) value_type(std::forward<_Args>(__args)...);
        m_end++;
    }

    void pop_back() { m_end--; }

private:
    void _realloc_array(size_type bytes) {
        if (bytes == 0) {
            std::free(m_start);
            m_start = m_end = m_cap = nullptr;
        } else {
            auto old_bytes = size_type(reinterpret_cast<char *>(m_end) -
                                       reinterpret_cast<char *>(m_start));
            auto *new_start =
                reinterpret_cast<pointer>(std::realloc(m_start, bytes));
            if (new_start) {
                m_start = new_start;
                m_cap = reinterpret_cast<pointer>(
                    reinterpret_cast<char *>(new_start) + bytes);
                m_end = reinterpret_cast<pointer>(
                    reinterpret_cast<char *>(new_start) + old_bytes);
            } else {
                throw std::bad_alloc();
            }
        }
    }

    value_type *m_start;
    value_type *m_end;
    value_type *m_cap;
};

template <typename T>
void swap(naivevector<T> &lhs, naivevector<T> &rhs) noexcept {
    lhs.swap(rhs);
}
} // namespace libime

#endif // NAIVEVECTOR_H
