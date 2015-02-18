/*
 * Project libime
 * Copyright (C) 2015  Xuetian Weng <wengxt@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LIBIME_DATRIE_H
#define LIBIME_DATRIE_H

#include <functional>
#include <memory>
#include <vector>
#include "common.h"

namespace libime
{

template<typename T>
class DATriePrivate;

template<typename T>
class DATrie;

template <typename T> struct NaN { enum { N1 = -1, N2 = -2 }; };
template <> struct NaN <float> { enum { N1 = 0x7f800001, N2 = 0x7f800002 }; };

template<typename T>
void swap(DATrie<T>& first, DATrie<T>& second) noexcept;

/**
 * This is a trie based on cedar<www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/>.
 *
 * comparing with original version, this version takes care of endianness when
 * doing serialization, and handle the possible unaligned memory access on some
 * platform.
 *
 * It also exploits the C++11 feature which can let you implement update easily
 * with lambda, comparing with the original version's add/set.
 *
 */
template<typename T>
class DATrie
{
public:
    typedef T value_type;
    typedef uint64_t position_type;
    typedef std::function< bool(value_type, size_t, position_type)> callback_type;
    typedef std::function<value_type(value_type)> updater_type;

    enum {
        NO_VALUE = NaN<value_type>::N1,
        NO_PATH = NaN<value_type>::N2
    };
    DATrie();
    DATrie(const DATrie<T>& other);
    DATrie(DATrie<T>&& other);
    DATrie(const char* filename);
    DATrie(std::istream& in);
    virtual ~DATrie();

    DATrie& operator=(DATrie other);

    friend void swap<>(DATrie& first, DATrie& second) noexcept;

    void save(const char* filename);
    void save(std::ostream& stream);

    size_t size() const;

    void suffix(std::string& s, size_t len, position_type pos);
    value_type exactMatchSearch(const std::string& key);
    value_type exactMatchSearch(const char* key, size_t len);
    DATrie< T >::value_type traverse(const std::string& key, position_type& from, size_t& pos);
    void set(const std::string& key, value_type val);
    void set(const char* key, size_t len, value_type val);
    void update(const std::string& key, updater_type updater);
    void update(const char* key, size_t len, updater_type updater);
    void dump(value_type* data, std::size_t size);
    void dump(std::vector<value_type>& data);
    void dump(std::vector<std::tuple<value_type, size_t, position_type>>& data);
    bool erase(const std::string& key, position_type from = 0);
    bool erase(position_type from = 0);

    void foreach(callback_type func);
    void foreach(const std::string& prefix, callback_type func, position_type pos = 0);
    void clear();
    void shrink_tail();

    static bool isValid(value_type v);
    static bool isNoPath(value_type v);
    static bool isNoValue(value_type v);

    size_t mem_size();

private:
    std::unique_ptr<DATriePrivate<value_type>> d;
};


template<typename T>
void swap(DATrie<T>& first, DATrie<T>& second) noexcept
{
    using std::swap;
    swap(first.d, second.d);
}

template class LIBIME_API DATrie<float>;
template class LIBIME_API DATrie<int32_t>;

}

#endif // LIBIME_DATRIE_H
