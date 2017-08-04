/*
* Copyright (C) 2015~2017 by CSSlayer
* wengxt@gmail.com
*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2 of the
* License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; see the file COPYING. If not,
* see <http://www.gnu.org/licenses/>.
*/

// Original Author
// cedar -- C++ implementation of Efficiently-updatable Double ARray trie
//  $Id: cedarpp.h 1830 2014-06-16 06:17:42Z ynaga $
// Copyright (c) 2009-2014 Naoki Yoshinaga <ynaga@tkl.iis.u-tokyo.ac.jp>

#include "datrie.h"
#include "naivevector.h"
#include "utils.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace libime {

#if 0
template<typename T>
using vector_impl = std::vector<T>;
#else
template <typename T>
using vector_impl = naivevector<T>;
#endif

template <typename V>
class DATriePrivate {
public:
    typedef DATrie<V> base_type;
    typedef typename base_type::value_type value_type;
    typedef typename base_type::position_type position_type;
    typedef typename base_type::updater_type updater_type;
    typedef typename base_type::callback_type callback_type;

    union decorder_type {
        int32_t result;
        value_type result_value;
    };

    enum error_code {
        CEDAR_NO_VALUE = base_type::NO_VALUE,
        CEDAR_NO_PATH = base_type::NO_PATH
    };
    static const int MAX_ALLOC_SIZE = 1 << 16; // must be divisible by 256
    const int MAX_TRIAL = 1;
    const bool ORDERED = true;
    typedef value_type result_type;
    typedef uint8_t uchar;
    static_assert(sizeof(value_type) <= sizeof(int32_t),
                  "value size need to be same as int32_t");
    struct node {
        union {
            int32_t base;
            value_type value;
        };
        int32_t check;
        node(const int32_t base_ = 0, const int32_t check_ = 0)
            : base(base_), check(check_) {}

        node(std::istream &in) {
            throw_if_io_fail(unmarshall(in, base));
            throw_if_io_fail(unmarshall(in, check));
        }

        node &operator=(const node &other) {
            base = other.base;
            check = other.check;
            return (*this);
        }

        friend std::ostream &operator<<(std::ostream &out, const node &n) {
            marshall(out, n.base) && marshall(out, n.check);
            return out;
        }
    };

    struct npos_t {
        uint32_t offset;
        uint32_t index;

        npos_t() : offset(0), index(0) {}

        explicit npos_t(position_type i) {
            offset = i >> 32;
            index = i & 0xffffffffULL;
        }

        operator bool() { return offset != 0 && index != 0; }

        bool operator==(const npos_t &other) const {
            return offset == other.offset && index == other.index;
        }

        bool operator!=(const npos_t &other) const {
            return !operator==(other);
        }

        position_type toInt() {
            return (static_cast<position_type>(offset) << 32) | index;
        }
    };
    struct block {      // a block w/ 256 elements
        int32_t prev;   // prev block; 3 bytes
        int32_t next;   // next block; 3 bytes
        int16_t num;    // # empty elements; 0 - 256
        int16_t reject; // minimum # branching failed to locate; soft limit
        int32_t trial;  // # trial
        int32_t ehead;  // first empty item
        block() : prev(0), next(0), num(256), reject(257), trial(0), ehead(0) {}

        block(std::istream &in) {
            throw_if_io_fail(unmarshall(in, prev));
            throw_if_io_fail(unmarshall(in, next));
            throw_if_io_fail(unmarshall(in, num));
            throw_if_io_fail(unmarshall(in, reject));
            throw_if_io_fail(unmarshall(in, trial));
            throw_if_io_fail(unmarshall(in, ehead));
        }

        friend std::ostream &operator<<(std::ostream &out, const block &b) {
            marshall(out, b.prev) && marshall(out, b.next) &&
                marshall(out, b.num) && marshall(out, b.reject) &&
                marshall(out, b.trial) && marshall(out, b.ehead);
            return out;
        }
    };
    struct ninfo {     // x1.5 update speed; +.25 % memory (8n -> 10n)
        uchar sibling; // right sibling (= 0 if not exist)
        uchar child;   // first child
        ninfo() : sibling(0), child(0) {}

        ninfo(std::istream &in) {
            throw_if_io_fail(unmarshall(in, sibling));
            throw_if_io_fail(unmarshall(in, child));
        }

        friend std::ostream &operator<<(std::ostream &out, const ninfo &n) {
            marshall(out, n.sibling) && marshall(out, n.child);
            return out;
        }
    };

    vector_impl<node> m_array;
    vector_impl<char> m_tail;
    vector_impl<int> m_tail0;
    vector_impl<block> m_block;
    vector_impl<ninfo> m_ninfo;

    int32_t m_bheadF; // first block of Full;   0
    int32_t m_bheadC; // first block of Closed; 0 if no Closed
    int32_t m_bheadO; // first block of Open;   0 if no Open
    std::array<int, 257> m_reject;

    DATriePrivate() { init(); }

    DATriePrivate(const DATriePrivate &other)
        : m_array(other.m_array), m_tail(other.m_tail), m_tail0(other.m_tail0),
          m_block(other.m_block), m_ninfo(other.m_ninfo),
          m_bheadF(other.m_bheadF), m_bheadC(other.m_bheadC),
          m_bheadO(other.m_bheadO), m_reject(other.m_reject) {}

    size_t size() const { return m_ninfo.size(); }

    size_t capacity() const { return m_array.size(); }

    void clear() {
        init();
        m_array.shrink_to_fit();
        m_block.shrink_to_fit();
        m_tail.shrink_to_fit();
        m_ninfo.shrink_to_fit();
        m_tail0.shrink_to_fit();
    }

    size_t num_keys() const {
        size_t i = 0;
        for (auto to = 0; to < static_cast<int>(size()); ++to) {
            const node &n = m_array[to];
            if (n.check >= 0 && (m_array[n.check].base == to || n.base < 0)) {
                ++i;
            }
        }
        return i;
    }

    void open(std::istream &fin) {
        uint32_t len = 0;
        uint32_t size_ = 0;
        throw_if_io_fail(unmarshall(fin, len));
        throw_if_io_fail(unmarshall(fin, size_));

        const size_t length_ = static_cast<size_t>(len);

        m_tail.resize(length_);
        m_tail0.resize(0);
        m_array.reserve(size_);
        m_array.resize(0);
        m_ninfo.reserve(size_);
        m_ninfo.resize(0);
        m_block.reserve(size_ >> 8);
        m_block.resize(0);

        throw_if_io_fail(fin.read(reinterpret_cast<char *>(m_tail.data()),
                                  sizeof(char) * length_));

        for (auto i = 0U; i < size_; i++) {
            m_array.emplace_back(fin);
        }
        m_array.resize(size_);

        throw_if_io_fail(unmarshall(fin, m_bheadF));
        throw_if_io_fail(unmarshall(fin, m_bheadC));
        throw_if_io_fail(unmarshall(fin, m_bheadO));

        for (auto i = 0U; i < size_; i++) {
            m_ninfo.emplace_back(fin);
        }

        for (auto i = 0U, end = size_ >> 8; i < end; i++) {
            m_block.emplace_back(fin);
        }
    }

    void save(std::ostream &fout) {
        shrink_tail();

        const uint32_t length = m_tail.size();
        const uint32_t size_ = size();

        assert(m_block.size() << 8 == m_ninfo.size());
        throw_if_io_fail(marshall(fout, length));
        throw_if_io_fail(marshall(fout, size_));
        throw_if_io_fail(fout.write(reinterpret_cast<char *>(m_tail.data()),
                                    sizeof(char) * length));

        auto s = size_;
        for (auto &n : m_array) {
            throw_if_io_fail(fout << n);
            if (--s == 0) {
                break;
            }
        }

        throw_if_io_fail(marshall(fout, m_bheadF));
        throw_if_io_fail(marshall(fout, m_bheadC));
        throw_if_io_fail(marshall(fout, m_bheadO));

        for (auto &n : m_ninfo) {
            throw_if_io_fail(fout << n);
        }

        for (auto &b : m_block) {
            throw_if_io_fail(fout << b);
        }
    }

    void init() {
        m_bheadF = m_bheadC = m_bheadO = 0;
        m_array.clear();
        m_array.resize(256);
        m_array[0] = node(0, -1);

        for (int i = 1; i < 256; ++i)
            m_array[i] =
                node(i == 1 ? -255 : -(i - 1), i == 255 ? -1 : -(i + 1));

        m_ninfo.clear();
        m_ninfo.resize(256);

        m_block.clear();
        m_block.reserve(1);
        m_block.resize(1);
        m_block[0].ehead = 1;
        // put a dummy entry
        m_tail0.resize(0);
        m_tail.clear();
        m_tail.resize(sizeof(int32_t));

        for (auto i = 0; i <= 256; ++i) {
            m_reject[i] = i + 1;
        }
    }

    void suffix(std::string &key, size_t len, npos_t pos) const {
        key.clear();
        key.resize(len);

        auto to = pos.index;
        if (const int offset = pos.offset) {
            size_t len_tail = std::strlen(&m_tail[-m_array[to].base]);
            if (len > len_tail) {
                len -= len_tail;
            } else {
                len_tail = len;
                len = 0;
            }
            std::copy(&m_tail[static_cast<size_t>(offset) - len_tail],
                      &m_tail[static_cast<size_t>(offset)], key.begin() + len);
        }
        while (len--) {
            const int from = m_array[to].check;
            key[len] =
                static_cast<char>(m_array[from].base ^ static_cast<int>(to));
            to = from;
        }
    }
    value_type traverse(const char *key, npos_t &from, size_t &pos) const {
        return traverse(key, from, pos, std::strlen(key));
    }

    value_type traverse(const char *key, npos_t &from, size_t &pos,
                        size_t len) const {
        decorder_type d;
        d.result = _find(key, from, pos, len);
        return d.result_value;
    }

    template <typename U>
    inline void update(const char *key, U &&callback) {
        update(key, std::strlen(key),
               std::forward<decltype(callback)>(callback));
    }

    template <typename U>
    inline void update(const char *key, size_t len, U &&callback) {
        npos_t from;
        size_t pos(0);
        update(key, from, pos, len, std::forward<decltype(callback)>(callback));
    }

    template <typename U>
    inline void update(const char *key, npos_t &from, size_t &pos, size_t len,
                       U &&callback) {
        update(key, from, pos, len, std::forward<decltype(callback)>(callback),
               [](const int, const int) {});
    }

    template <typename U, typename T>
    void update(const char *key, npos_t &npos, size_t &pos, size_t len,
                U &&callback, T &&cf) {
        if (!len && !npos) {
            throw std::invalid_argument("failed to insert zero-length key");
        }

        auto &from = npos.index;
        auto offset = npos.offset;
        if (!offset) { // node on trie
            for (const uchar *const key_ = reinterpret_cast<const uchar *>(key);
                 m_array[from].base >= 0; ++pos) {
                if (pos == len) {
                    const auto to = _follow(from, 0, cf);
                    m_array[to].value = callback(m_array[to].value);
                    return;
                }
                from = static_cast<size_t>(_follow(from, key_[pos], cf));
            }
            offset = -m_array[from].base;
        }
        if (offset >= sizeof(int32_t)) { // go to m_tail
            const size_t pos_orig = pos;
            char *const tail = m_tail.data() + offset - pos;
            while (pos < len && key[pos] == tail[pos]) {
                ++pos;
            }
            //
            if (pos == len && tail[pos] == '\0') { // found exact key
                if (const ssize_t moved =
                        pos - pos_orig) { // search end on tail
                    npos.offset = offset + moved;
                }
                char *const data = &tail[len + 1];
                store_data(data, callback(load_data<value_type>(data)));
                return;
            }
            // otherwise, insert the common prefix in tail if any
            if (npos.offset) {
                npos.offset = 0; // reset to update tail offset
                for (auto offset_ = static_cast<size_t>(-m_array[from].base);
                     offset_ < offset;) {
                    from = static_cast<size_t>(
                        _follow(from, static_cast<uchar>(m_tail[offset_]), cf));
                    ++offset_;
                }
            }
            for (size_t pos_ = pos_orig; pos_ < pos; ++pos_) {
                from = static_cast<size_t>(
                    _follow(from, static_cast<uchar>(key[pos_]), cf));
            }
            // for example:
            // we originally had abcde in trie, then bcde is in tail
            // if we want to insert abcdf
            // pos would be 1 (things on trie that matched)
            // and pos_orig will be 4 (things on tail + trie) that matched
            // and tail[pos] will not be empty in this case
            ssize_t moved = pos - pos_orig;
            if (tail[pos]) { // remember to move offset to existing tail
                const int to_ =
                    _follow(from, static_cast<uchar>(tail[pos]), cf);
                m_array[to_].base = -static_cast<int>(offset + ++moved);
                moved -= 1 + sizeof(value_type); // keep record
            }
            moved += offset;
            for (auto i = offset; i <= moved; i += 1 + sizeof(value_type)) {
                if (m_tail0.capacity() == m_tail0.size()) {
                    auto quota =
                        m_tail0.capacity() + (m_tail0.size() >= MAX_ALLOC_SIZE
                                                  ? MAX_ALLOC_SIZE
                                                  : m_tail0.size());
                    m_tail0.reserve(quota);
                }
                m_tail0.push_back(i);
            }
            if (pos == len || tail[pos] == '\0') {
                const int to = _follow(from, 0, cf);
                if (pos == len) {
                    m_array[to].value = callback(m_array[to].value);
                    return;
                }
                // tail[pos] == 0, so the actual content in tail can be get rid
                // of
                // and tail0 is used to track those hole
                m_array[to].value = load_data<value_type>(&tail[pos + 1]);
            }
            from = static_cast<size_t>(
                _follow(from, static_cast<uchar>(key[pos]), cf));
            ++pos;
        }
        const auto needed = len - pos + 1 + sizeof(value_type);
        if (pos == len && m_tail0.size() > 0) { // reuse
            const int offset0 = *m_tail0.rbegin();
            m_tail[offset0] = '\0';
            m_array[from].base = -offset0;
            m_tail0.pop_back();
            char *const data = &m_tail[offset0 + 1];
            store_data(data, callback(0));
            return;
        }
        if (m_tail.capacity() < m_tail.size() + needed) {
            auto quota =
                m_tail.capacity() +
                ((needed > m_tail.size() || needed > MAX_ALLOC_SIZE)
                     ? needed
                     : (m_tail.size() >= MAX_ALLOC_SIZE ? MAX_ALLOC_SIZE
                                                        : m_tail.size()));
            m_tail.reserve(quota);
        }
        m_array[from].base = -m_tail.size();
        const size_t pos_orig = pos;
        auto old_length = m_tail.size();
        m_tail.resize(m_tail.size() + needed);
        char *const tail = &m_tail[old_length] - pos;
        if (pos < len) {
            do {
                tail[pos] = key[pos];
            } while (++pos < len);
            npos.offset = old_length + len - pos_orig;
        }
        char *const data = &tail[len + 1];
        store_data(data, callback(load_data<value_type>(data)));
        return;
    }

    // easy-going erase () without compression
    int erase(const char *key) { return erase(key, std::strlen(key)); }
    int erase(const char *key, size_t len, npos_t npos = npos_t()) {
        size_t pos = 0;
        auto &from = npos.index;
        const auto i = _find(key, npos, pos, len);
        if (i == CEDAR_NO_PATH || i == CEDAR_NO_VALUE) {
            return -1;
        }
        if (npos.offset) {
            npos.offset = 0; // leave tail as is
        }
        bool flag = m_array[from].base < 0; // have sibling
        int e = flag ? static_cast<int>(from) : m_array[from].base ^ 0;
        from = m_array[e].check;
        do {
            const node &n = m_array[from];
            flag = m_ninfo[n.base ^ m_ninfo[from].child].sibling;
            if (flag) {
                _pop_sibling(from, n.base, static_cast<uchar>(n.base ^ e));
            }
            _push_enode(e);
            e = static_cast<int>(from);
            from = static_cast<size_t>(m_array[from].check);
        } while (!flag);
        return 0;
    }

    void foreach (callback_type callback, npos_t root = npos_t()) const {
        decorder_type b;
        size_t p(0);
        npos_t from = root;
        for (b.result = begin(from, p); b.result != CEDAR_NO_PATH;
             b.result = next(from, p, root)) {
            if (b.result != CEDAR_NO_VALUE &&
                !callback(b.result_value, p, from.toInt())) {
                break;
            }
        }
    }

    template <typename T>
    void dump(T *result, const size_t result_len) const {
        size_t num(0);
        foreach ([result, result_len, &num](value_type value, size_t len,
                                            position_type pos) {
            if (num < result_len) {
                _set_result(&result[num++], value, len, npos_t(pos));
            } else {
                return false;
            }
            return true;
        })
            ;
    }
    void shrink_tail() {
        const size_t length_ =
            static_cast<size_t>(m_tail.size()) -
            static_cast<size_t>(m_tail0.size()) * (1 + sizeof(value_type));
        decltype(m_tail) t;
        // a dummy entry
        t.resize(sizeof(int32_t));
        t.reserve(length_);
        for (int to = 0; to < static_cast<int>(size()); ++to) {
            node &n = m_array[to];
            if (n.check >= 0 && m_array[n.check].base != to && n.base < 0) {
                char *const tail_(&m_tail[-n.base]);
                n.base = -static_cast<int32_t>(t.size());
                auto i = 0;
                do {
                    t.push_back(tail_[i]);
                } while (tail_[i++]);
                t.resize(t.size() + sizeof(value_type));
                store_data(&t[t.size() - sizeof(value_type)],
                           load_data<value_type>(&tail_[i]));
            }
        }
        using std::swap;
        swap(t, m_tail);
        m_tail0.resize(0);
        m_tail0.shrink_to_fit();
    }

    // return the first child for a tree rooted by a given node
    int32_t begin(npos_t &npos, size_t &len) const {
        auto &from = npos.index;
        int base =
            npos.offset ? -static_cast<int>(npos.offset) : m_array[from].base;
        if (base >= 0) { // on trie
            uchar c = m_ninfo[from].child;
            if (!from && !(c = m_ninfo[base ^ c].sibling)) { // bug fix
                return CEDAR_NO_PATH;                        // no entry
            }
            for (; c && base >= 0; ++len) {
                from = static_cast<size_t>(base) ^ c;
                base = m_array[from].base;
                c = m_ninfo[from].child;
            }
            if (base >= 0) {
                return m_array[base ^ c].base;
            }
        }
        const size_t len_ = std::strlen(&m_tail[-base]);
        npos.offset = static_cast<size_t>(-base) + len_;
        len += len_;
        return load_data<int32_t>(&m_tail[-base] + len_ + 1);
    }
    // return the next child if any
    int32_t next(npos_t &npos, size_t &len,
                 const npos_t root = npos_t()) const {
        uchar c = 0;
        auto &from = npos.index;
        if (const int offset = npos.offset) { // on tail
            if (root.offset) {
                return CEDAR_NO_PATH;
            }
            npos.offset = 0;
            len -= static_cast<size_t>(offset - (-m_array[from].base));
        } else {
            c = m_ninfo[m_array[from].base].sibling;
        }
        for (; !c && npos != root; --len) {
            c = m_ninfo[from].sibling;
            from = static_cast<size_t>(m_array[from].check);
        }
        if (!c) {
            return CEDAR_NO_PATH;
        }
        from = static_cast<size_t>(m_array[from].base) ^ c;
        return begin(npos, ++len);
    }
    // follow/create edge
    template <typename T>
    int _follow(uint32_t &from, const uchar label, T cf) {
        int to = 0;
        const int base = m_array[from].base;
        if (base < 0 || m_array[to = base ^ label].check < 0) {
            to = _pop_enode(base, label, static_cast<int>(from));
            _push_sibling(from, to ^ label, label, base >= 0);
        } else if (m_array[to].check != static_cast<int>(from)) {
            to = _resolve(from, base, label, cf);
        }
        return to;
    }

    // find key from double array
    int32_t _find(const char *key, npos_t &npos, size_t &pos,
                  const size_t len) const {
        auto &from = npos.index;
        auto offset = npos.offset;
        if (!offset) { // node on trie
            for (const uchar *const key_ = reinterpret_cast<const uchar *>(key);
                 m_array[from].base >= 0;) {
                if (pos == len) {
                    const node &n = m_array[m_array[from].base ^ 0];
                    if (n.check != static_cast<int>(from)) {
                        return CEDAR_NO_VALUE;
                    }
                    return n.base;
                }
                size_t to = static_cast<size_t>(m_array[from].base);
                to ^= key_[pos];
                if (m_array[to].check != static_cast<int>(from)) {
                    return CEDAR_NO_PATH;
                }
                ++pos;
                from = to;
            }
            offset = -m_array[from].base;
        }
        // switch to _tail to match suffix
        const size_t pos_orig = pos; // start position in reading _tail
        const char *const tail = &m_tail[offset] - pos;
        if (pos < len) {
            do {
                if (key[pos] != tail[pos])
                    break;
            } while (++pos < len);
            if (const int moved = pos - pos_orig) {
                npos.offset = offset + moved;
            }
            if (pos < len) {
                return CEDAR_NO_PATH; // input > tail, input != tail
            }
        }
        if (tail[pos]) {
            return CEDAR_NO_VALUE; // input < tail
        }
        return load_data<int32_t>(&tail[len + 1]);
    }

    // explore new block to settle down
    int _find_place() {
        if (m_bheadC) {
            return m_block[m_bheadC].ehead;
        }
        if (m_bheadO) {
            return m_block[m_bheadO].ehead;
        }
        return _add_block() << 8;
    }
    int _find_place(const uchar *const first, const uchar *const last) {
        if (auto bi = m_bheadO) {
            const auto bz = m_block[m_bheadO].prev;
            const auto nc = static_cast<short>(last - first + 1);
            while (1) { // set candidate block
                block &b = m_block[bi];
                if (b.num >= nc && nc < b.reject) // explore configuration
                    for (int e = b.ehead;;) {
                        const int base = e ^ *first;
                        for (const uchar *p = first;
                             m_array[base ^ *++p].check < 0;) {
                            if (p == last) {
                                return b.ehead = e; // no conflict
                            }
                        }
                        if ((e = -m_array[e].check) == b.ehead)
                            break;
                    }
                b.reject = nc;
                if (b.reject < m_reject[b.num]) {
                    m_reject[b.num] = b.reject;
                }
                const int bi_ = b.next;
                if (++b.trial == MAX_TRIAL) {
                    _transfer_block(bi, m_bheadO, m_bheadC);
                }
                if (bi == bz)
                    break;
                bi = bi_;
            }
        }
        return _add_block() << 8;
    }

    static void _set_result(result_type *x, value_type r, size_t = 0,
                            npos_t = npos_t()) {
        *x = r;
    }
    static void _set_result(std::tuple<value_type, size_t, position_type> *x,
                            value_type r, size_t len, npos_t npos) {
        *x = std::make_tuple<>(r, len, npos.toInt());
    }
    void _pop_block(const int bi, int &head_in, const bool last) {
        if (last) { // last one poped; Closed or Open
            head_in = 0;
        } else {
            const block &b = m_block[bi];
            m_block[b.prev].next = b.next;
            m_block[b.next].prev = b.prev;
            if (bi == head_in) {
                head_in = b.next;
            }
        }
    }
    void _push_block(const int bi, int &head_out, const bool empty) {
        block &b = m_block[bi];
        if (empty) { // the destination is empty
            head_out = b.prev = b.next = bi;
        } else { // use most recently pushed
            int &tail_out = m_block[head_out].prev;
            b.prev = tail_out;
            b.next = head_out;
            head_out = tail_out = m_block[tail_out].next = bi;
        }
    }
    int _add_block() {
        // size depends on m_info.size()
        if (size() == capacity()) { // allocate memory if needed
            auto new_capacity =
                capacity() +
                (size() >= MAX_ALLOC_SIZE ? MAX_ALLOC_SIZE : size());
            m_array.reserve(new_capacity);
            m_array.resize(new_capacity);
            m_ninfo.reserve(new_capacity);
            m_block.reserve(new_capacity >> 8);
            m_block.resize(size() >> 8);
        }
        assert(m_block.size() == size() >> 8);
        m_block.resize(m_block.size() + 1);
        m_block[size() >> 8].ehead = size();

        assert(m_array.size() >= size() + 256);
        m_array[size()] = node(-(size() + 255), -(size() + 1));
        for (auto i = size() + 1; i < size() + 255; ++i) {
            m_array[i] = node(-(i - 1), -(i + 1));
        }
        m_array[size() + 255] = node(-(size() + 254), -size());
        _push_block(size() >> 8, m_bheadO, !m_bheadO); // append to block Open
        m_ninfo.resize(size() + 256);
        return (size() >> 8) - 1;
    }
    // transfer block from one start w/ head_in to one start w/ head_out
    void _transfer_block(const int bi, int &head_in, int &head_out) {
        _pop_block(bi, head_in, bi == m_block[bi].next);
        _push_block(bi, head_out, !head_out && m_block[bi].num);
    }
    // pop empty node from block; never transfer the special block (bi = 0)
    int _pop_enode(const int base, const uchar label, const int from) {
        const int e = base < 0 ? _find_place() : base ^ label;
        const int bi = e >> 8;
        node &n = m_array[e];
        block &b = m_block[bi];
        if (--b.num == 0) {
            if (bi) {
                _transfer_block(bi, m_bheadC, m_bheadF); // Closed to Full
            }
        } else { // release empty node from empty ring
            m_array[-n.base].check = n.check;
            m_array[-n.check].base = n.base;
            if (e == b.ehead)
                b.ehead = -n.check; // set ehead
            if (bi && b.num == 1 && b.trial != MAX_TRIAL) {
                // Open to Closed
                _transfer_block(bi, m_bheadO, m_bheadC);
            }
        }
        // initialize the released node
        if (label) {
            n.base = -1;
        } else {
            n.value = value_type(0);
        }
        n.check = from;
        if (base < 0) {
            m_array[from].base = e ^ label;
        }
        return e;
    }
    // push empty node into empty ring
    void _push_enode(const int e) {
        const int bi = e >> 8;
        block &b = m_block[bi];
        if (++b.num == 1) { // Full to Closed
            b.ehead = e;
            m_array[e] = node(-e, -e);
            if (bi) {
                _transfer_block(bi, m_bheadF, m_bheadC); // Full to Closed
            }
        } else {
            const int prev = b.ehead;
            const int next = -m_array[prev].check;
            m_array[e] = node(-prev, -next);
            m_array[prev].check = m_array[next].base = -e;
            if (b.num == 2 || b.trial == MAX_TRIAL) { // Closed to Open
                if (bi) {
                    _transfer_block(bi, m_bheadC, m_bheadO);
                }
            }
            b.trial = 0;
        }
        if (b.reject < m_reject[b.num])
            b.reject = m_reject[b.num];
        m_ninfo[e] = ninfo(); // reset ninfo; no child, no sibling
    }
    // push label to from's child
    void _push_sibling(const int32_t from, const int base, const uchar label,
                       const bool flag = true) {
        uchar *c = &m_ninfo[from].child;
        if (flag && (ORDERED ? label > *c : !*c)) {
            do {
                c = &m_ninfo[base ^ *c].sibling;
            } while (ORDERED && *c && *c < label);
        }
        m_ninfo[base ^ label].sibling = *c, *c = label;
    }
    // pop label from from's child
    void _pop_sibling(const int32_t from, const int base, const uchar label) {
        uchar *c = &m_ninfo[from].child;
        while (*c != label) {
            c = &m_ninfo[base ^ *c].sibling;
        }
        *c = m_ninfo[base ^ label].sibling;
    }
    // check whether to replace branching w/ the newly added node
    bool _consult(const int base_n, const int base_p, uchar c_n,
                  uchar c_p) const {
        do {
            c_n = m_ninfo[base_n ^ c_n].sibling;
            c_p = m_ninfo[base_p ^ c_p].sibling;
        } while (c_n && c_p);
        return c_p;
    }
    // enumerate (equal to or more than one) child nodes
    uchar *_set_child(uchar *p, const int base, uchar c, const int label = -1) {
        --p;
        if (!c) {
            *++p = c;
            c = m_ninfo[base ^ c].sibling;
        } // 0: terminal
        if (ORDERED) {
            while (c && c < label) {
                *++p = c;
                c = m_ninfo[base ^ c].sibling;
            }
        }
        if (label != -1) {
            *++p = static_cast<uchar>(label);
        }
        while (c) {
            *++p = c;
            c = m_ninfo[base ^ c].sibling;
        }
        return p;
    }
    // resolve conflict on base_n ^ label_n = base_p ^ label_p
    template <typename T>
    int _resolve(uint32_t &from_n, const int base_n, const uchar label_n,
                 T cf) {
        // examine siblings of conflicted nodes
        const int to_pn = base_n ^ label_n;
        const int from_p = m_array[to_pn].check;
        const int base_p = m_array[from_p].base;
        const bool flag // whether to replace siblings of newly added
            = _consult(base_n, base_p, m_ninfo[from_n].child,
                       m_ninfo[from_p].child);
        uchar child[256];
        uchar *const first = &child[0];
        uchar *const last =
            flag ? _set_child(first, base_n, m_ninfo[from_n].child, label_n)
                 : _set_child(first, base_p, m_ninfo[from_p].child);
        const int base =
            (first == last ? _find_place() : _find_place(first, last)) ^ *first;
        // replace & modify empty list
        const int from = flag ? static_cast<int>(from_n) : from_p;
        const int base_ = flag ? base_n : base_p;
        if (flag && *first == label_n) {
            m_ninfo[from].child = label_n; // new child
        }
        m_array[from].base = base;                     // new base
        for (const uchar *p = first; p <= last; ++p) { // to_ => to
            const int to = _pop_enode(base, *p, from);
            const int to_ = base_ ^ *p;
            m_ninfo[to].sibling = (p == last ? 0 : *(p + 1));
            if (flag && to_ == to_pn)
                continue; // skip newcomer (no child)
            cf(to_, to);
            node &n = m_array[to];
            node &n_ = m_array[to_];
            if ((n.base = n_.base) > 0 && *p) { // copy base; bug fix
                uchar c = m_ninfo[to].child = m_ninfo[to_].child;
                do {
                    m_array[n.base ^ c].check = to; // adjust grand son's check
                } while ((c = m_ninfo[n.base ^ c].sibling));
            }
            if (!flag && to_ == static_cast<int>(from_n)) {
                // parent node moved
                from_n = static_cast<size_t>(to); // bug fix
            }
            if (!flag && to_ == to_pn) { // the address is immediately used
                _push_sibling(from_n, to_pn ^ label_n, label_n);
                m_ninfo[to_].child = 0; // remember to reset child
                if (label_n)
                    n_.base = -1;
                else
                    n_.value = value_type(0);
                n_.check = static_cast<int>(from_n);
            } else {
                _push_enode(to_);
            }
        }
        return flag ? base ^ label_n : to_pn;
    }
};

template <typename T>
DATrie<T>::DATrie() : d(new DATriePrivate<value_type>()) {}

template <typename T>
DATrie<T>::DATrie(const char *filename) : DATrie() {
    std::ifstream fin(filename, std::ios::in | std::ios::binary);
    throw_if_io_fail(fin);
    d->open(fin);
}

template <typename T>
DATrie<T>::DATrie(std::istream &fin) : DATrie() {
    d->open(fin);
}

template <typename T>
DATrie<T>::~DATrie() {}

template <typename T>
DATrie<T>::DATrie(DATrie<T> &&other) : d(std::move(other.d)) {}

template <typename T>
DATrie<T>::DATrie(const DATrie<T> &other) : d(new DATriePrivate<T>(*other.d)) {}

template <typename T>
DATrie<T> &DATrie<T>::operator=(DATrie<T> other) {
    swap(*this, other);
    return *this;
}

template <typename T>
void DATrie<T>::load(std::istream &in) {
    clear();
    d->open(in);
}

template <typename T>
void DATrie<T>::save(const char *filename) {
    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    throw_if_io_fail(fout);
    save(fout);
}

template <typename T>
void DATrie<T>::save(std::ostream &stream) {
    d->save(stream);
}

template <typename T>
void DATrie<T>::set(const char *key, size_t len, value_type val) {
    d->update(key, len, [val](value_type) { return val; });
}

template <typename T>
void DATrie<T>::update(const char *key, size_t len,
                       DATrie<T>::updater_type updater) {
    d->update(key, len, updater);
}

template <typename T>
size_t DATrie<T>::size() const {
    return d->num_keys();
}

template <typename T>
void DATrie<T>::foreach (const char *key, size_t size, callback_type func,
                         position_type _pos) const {
    size_t pos = 0;
    typename DATriePrivate<value_type>::npos_t from(_pos);
    if (d->_find(key, from, pos, size) == NO_PATH) {
        return;
    }

    d->foreach (func, from);
}

template <typename T>
void DATrie<T>::foreach (callback_type func, position_type pos) const {
    typename DATriePrivate<value_type>::npos_t from(pos);
    d->foreach (func, from);
}

template <typename T>
void DATrie<T>::suffix(std::string &s, size_t len, position_type pos) const {
    d->suffix(s, len, typename DATriePrivate<T>::npos_t(pos));
}

template <typename T>
void DATrie<T>::dump(value_type *data, std::size_t size) const {
    d->dump(data, size);
}

template <typename T>
void DATrie<T>::dump(std::vector<typename DATrie<T>::value_type> &data) const {
    data.resize(size());
    d->dump(data.data(), data.size());
}

template <typename T>
void DATrie<T>::dump(
    std::vector<std::tuple<typename DATrie<T>::value_type, size_t,
                           typename DATrie<T>::position_type>> &data) const {
    data.resize(size());
    d->dump(data.data(), data.size());
}

template <typename T>
bool DATrie<T>::erase(const char *key, size_t len, position_type from) {
    return d->erase(key, len, typename DATriePrivate<T>::npos_t(from)) == 0;
}

template <typename T>
bool DATrie<T>::erase(position_type from) {
    return d->erase("", 0, typename DATriePrivate<T>::npos_t(from)) == 0;
}

template <typename T>
typename DATrie<T>::value_type DATrie<T>::exactMatchSearch(const char *key,
                                                           size_t len) const {
    size_t pos = 0;
    typename DATriePrivate<value_type>::npos_t npos;
    typename DATriePrivate<T>::decorder_type decoder;
    decoder.result = d->_find(key, npos, pos, len);
    if (decoder.result == NO_PATH) {
        decoder.result = NO_VALUE;
    }
    return decoder.result_value;
}

template <typename T>
typename DATrie<T>::value_type DATrie<T>::traverse(const char *key, size_t len,
                                                   position_type &from) const {
    size_t pos = 0;
    typename DATriePrivate<T>::npos_t npos(from);
    auto result = d->traverse(key, npos, pos, len);
    from = npos.toInt();
    return result;
}

template <typename T>
void DATrie<T>::clear() {
    d->clear();
}

template <typename T>
void DATrie<T>::shrink_tail() {
    d->shrink_tail();
}

template <typename T>
bool DATrie<T>::isNoPath(value_type v) {
    typename DATriePrivate<T>::decorder_type d;
    d.result_value = v;
    return d.result == NO_PATH;
}

template <typename T>
bool DATrie<T>::isNoValue(value_type v) {
    typename DATriePrivate<T>::decorder_type d;
    d.result_value = v;
    return d.result == NO_VALUE;
}

template <typename T>
bool DATrie<T>::isValid(value_type v) {
    return !(isNoPath(v) || isNoValue(v));
}

template <typename T>
size_t DATrie<T>::mem_size() const {
    //     std::cout << "tail" << d->m_tail.size() << std::endl
    //               << "tail0" << d->m_tail0.size() * sizeof(int) << std::endl
    //               << "array" << sizeof(typename
    //               decltype(d->m_array)::value_type) *
    //               d->m_array.size() << std::endl
    //               << "block" << sizeof(typename
    //               decltype(d->m_block)::value_type) *
    //               d->m_block.size() << std::endl
    //               << "ninfo" << sizeof(typename
    //               decltype(d->m_ninfo)::value_type) *
    //               d->m_ninfo.size() << std::endl;
    return d->m_tail.size() + d->m_tail0.size() * sizeof(int) +
           sizeof(typename decltype(d->m_array)::value_type) *
               d->m_array.size() +
           sizeof(typename decltype(d->m_block)::value_type) *
               d->m_block.size() +
           sizeof(typename decltype(d->m_ninfo)::value_type) *
               d->m_ninfo.size();
}
}
