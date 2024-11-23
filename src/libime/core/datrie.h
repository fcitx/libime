/*
 * SPDX-FileCopyrightText: 2015-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef _FCITX_LIBIME_CORE_DATRIE_H_
#define _FCITX_LIBIME_CORE_DATRIE_H_

/// \file
/// \brief Provide a DATrie implementation.

#include "libimecore_export.h"

#include <cstddef>
#include <cstdint>
#include <fcitx-utils/macros.h>
#include <functional>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace libime {

template <typename V, bool ORDERED = true, int MAX_TRIAL = 1>
class DATriePrivate;

template <typename T>
struct NaN {
    static constexpr auto N1 = -1;
    static constexpr auto N2 = -2;
};
template <>
struct NaN<float> {
    static constexpr auto N1 = 0x7fc00001;
    static constexpr auto N2 = 0x7fc00002;
};

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
template <typename T>
class DATrie {
public:
    using value_type = T;
    using position_type = uint64_t;
    using callback_type =
        std::function<bool(value_type, size_t, position_type)>;
    using updater_type = std::function<value_type(value_type)>;

    /*
     * The actual value may be different on different CPU, use isNoValue
     * isNoPath, or isValid instead.
     */
    enum {
        NO_VALUE LIBIMECORE_DEPRECATED = NaN<value_type>::N1,
        NO_PATH LIBIMECORE_DEPRECATED = NaN<value_type>::N2
    };
    DATrie();
    DATrie(const char *filename);
    DATrie(std::istream &in);

    FCITX_DECLARE_VIRTUAL_DTOR_COPY_AND_MOVE(DATrie)

    void load(std::istream &in);
    void save(const char *filename);
    void save(std::ostream &stream);

    size_t size() const;
    bool empty() const;

    // retrieve the string via len and pos
    void suffix(std::string &s, size_t len, position_type pos) const;

    // result will be NO_VALUE
    value_type exactMatchSearch(const char *key, size_t len) const;
    value_type exactMatchSearch(std::string_view key) const {
        return exactMatchSearch(key.data(), key.size());
    }

    bool hasExactMatch(std::string_view key) const;

    DATrie<T>::value_type traverse(std::string_view key,
                                   position_type &from) const {
        return traverse(key.data(), key.size(), from);
    }
    DATrie<T>::value_type traverse(const char *key, size_t len,
                                   position_type &from) const;

    // set value
    void set(std::string_view key, value_type val) {
        return set(key.data(), key.size(), val);
    }
    void set(const char *key, size_t len, value_type val);

    void update(std::string_view key, updater_type updater) {
        update(key.data(), key.size(), updater);
    }
    void update(const char *key, size_t len, updater_type updater);

    void dump(value_type *data, std::size_t size) const;
    void dump(std::vector<value_type> &data) const;
    void dump(
        std::vector<std::tuple<value_type, size_t, position_type>> &data) const;

    // remove key
    bool erase(std::string_view key, position_type from = 0) {
        return erase(key.data(), key.size(), from);
    }
    bool erase(const char *key, size_t len, position_type from = 0);
    bool erase(position_type from = 0);

    // call callback on each key
    bool foreach(callback_type func, position_type pos = 0) const;
    // search by prefix
    bool foreach(const char *prefix, size_t size, callback_type func,
                 position_type pos = 0) const;
    bool foreach(std::string_view prefix, callback_type func,
                 position_type pos = 0) const {
        return foreach(prefix.data(), prefix.size(), func, pos);
    }
    void clear();
    void shrink_tail();

    static bool isValid(value_type v);
    static bool isNoPath(value_type v);
    static bool isNoValue(value_type v);

    static value_type noPath();
    static value_type noValue();

    size_t mem_size() const;

private:
    std::unique_ptr<DATriePrivate<value_type>> d;
};

extern template class LIBIMECORE_EXPORT DATrie<float>;
extern template class LIBIMECORE_EXPORT DATrie<int32_t>;
extern template class LIBIMECORE_EXPORT DATrie<uint32_t>;
} // namespace libime

#endif // _FCITX_LIBIME_CORE_DATRIE_H_
