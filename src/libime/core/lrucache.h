/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_LRU_H_
#define _FCITX_LIBIME_CORE_LRU_H_

#include <boost/container_hash/hash.hpp>
#include <boost/unordered_map.hpp>
#include <cstddef>
#include <list>
#include <utility>

namespace libime {

// A simple LRU cache.
template <typename K, typename V, typename H = boost::hash<K>>
class LRUCache {
public:
    using key_type = K;
    using value_type = V;
    // we use boost's unordered_map is for the heterogeneous lookup
    // functionality.
    using dict_type =
        boost::unordered_map<K, std::pair<V, typename std::list<K>::iterator>,
                             H>;

    LRUCache(size_t sz = 80) : sz_(sz) {}

    size_t size() const { return dict_.size(); }

    size_t capacity() const { return sz_; }

    bool empty() const { return dict_.empty(); }

    bool contains(const key_type &key) {
        return dict_.find(key) != dict_.end();
    }

    template <typename... Args>
    value_type *insert(const key_type &key, Args &&...args) {
        auto iter = dict_.find(key);
        if (iter == dict_.end()) {
            if (size() >= sz_) {
                evict();
            }

            order_.push_front(key);
            auto r = dict_.emplace(
                key, std::make_pair(value_type(std::forward<Args>(args)...),
                                    order_.begin()));
            return &r.first->second.first;
        }
        return nullptr;
    }

    void erase(const key_type &key) {
        auto i = dict_.find(key);
        if (i == dict_.end()) {
            return;
        }
        order_.erase(i->second.second);
        dict_.erase(i);
    }

    // find will refresh the item, so it is not const.
    value_type *find(const key_type &key) {
        // lookup value in the cache
        auto i = dict_.find(key);
        return find_helper(i);
    }

    template <class CompatibleKey, class CompatibleHash,
              class CompatiblePredicate>
    value_type *find(CompatibleKey const &k, CompatibleHash const &h,
                     CompatiblePredicate const &p) {
        return find_helper(dict_.find(k, h, p));
    }

    void clear() {
        dict_.clear();
        order_.clear();
    }

private:
    void evict() {
        // evict item from the end of most recently used list
        auto i = std::prev(order_.end());
        dict_.erase(*i);
        order_.erase(i);
    }

    value_type *find_helper(typename dict_type::iterator i) {
        if (i == dict_.end()) {
            // value not in cache
            return nullptr;
        }

        // return the value, but first update its place in the most
        // recently used list
        auto j = i->second.second;
        if (j != order_.begin()) {
            order_.splice(order_.begin(), order_, j, std::next(j));
            j = order_.begin();
            i->second.second = j;
        }
        return &i->second.first;
    }

    dict_type dict_;
    std::list<K> order_;
    // Maximum size of the cache.
    size_t sz_;
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_LRU_H_
