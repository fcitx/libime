/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_LIBIME_PINYINENCODER_H_
#define _FCITX_LIBIME_PINYINENCODER_H_

#include "libime_export.h"
#include <boost/utility/string_view.hpp>
#include <cassert>
#include <fcitx-utils/flags.h>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace libime {

enum class PinyinFuzzyFlag {
    None = 0,
    NG_GN = 1 << 0,
    V_U = 1 << 1,
    AN_ANG = 1 << 2,   // 0
    EN_ENG = 1 << 3,   // 1
    IAN_IANG = 1 << 4, // 2
    IN_ING = 1 << 5,   // 3
    U_OU = 1 << 6,     // 4
    UAN_UANG = 1 << 7, // 5
    C_CH = 1 << 8,     // 0
    F_H = 1 << 9,      // 1
    L_N = 1 << 10,     // 2
    S_SH = 1 << 11,    // 3
    Z_ZH = 1 << 12,    // 4
    VE_UE = 1 << 13,
    Inner = 1 << 14,
};

using PinyinFuzzyFlags = fcitx::Flags<PinyinFuzzyFlag>;

enum class PinyinInitial : char {
    Invalid = 0,
    B = 'A',
    P,
    M,
    F,
    D,
    T,
    N,
    L,
    G,
    K,
    H,
    J,
    Q,
    X,
    ZH,
    CH,
    SH,
    R,
    Z,
    C,
    S,
    Y,
    W,
    Zero
};

enum class PinyinFinal : char {
    Invalid = 0,
    A = 'A',
    AI,
    AN,
    ANG,
    AO,
    E,
    EI,
    EN,
    ENG,
    ER,
    O,
    ONG,
    OU,
    I,
    IA,
    IE,
    IAO,
    IU,
    IAN,
    IN,
    IANG,
    ING,
    IONG,
    U,
    UA,
    UO,
    UAI,
    UI,
    UAN,
    UN,
    UANG,
    V,
    VE,
    UE,
    NG,
    Zero
};

class PinyinSegments;
typedef std::function<bool(const PinyinSegments &, const std::vector<size_t> &)>
    PinyinSegmentCallback;

class PinyinSegments {
public:
    PinyinSegments(const std::string &data = {}) : data_(data) {}
    PinyinSegments(const PinyinSegments &seg) = default;
    ~PinyinSegments() {}

    size_t start() const { return 0; }
    size_t end() const { return data_.size(); }

    boost::string_view pinyin() const { return data_; }

    boost::string_view segment(size_t start, size_t end) const {
        assert(start < end);
        return boost::string_view(data_.data() + start, end - start);
    }

    const std::vector<size_t> &next(size_t idx) const {
        auto iter = next_.find(idx);
        return iter->second;
    }
    void addNext(size_t from, size_t to) {
        assert(from < to);
        next_[from].push_back(to);
    }

    void dfs(PinyinSegmentCallback callback) const {
        std::vector<size_t> path;
        dfsHelper(path, 0, callback);
    }

private:
    bool dfsHelper(std::vector<size_t> &path, size_t start,
                   PinyinSegmentCallback callback) const {
        if (start == end()) {
            return callback(*this, path);
        }
        auto &nexts = next(start);
        for (auto next : nexts) {
            path.push_back(next);
            if (!dfsHelper(path, next, callback)) {
                return false;
            }
            path.pop_back();
        }
        return true;
    }

    std::string data_;
    std::unordered_map<size_t, std::vector<size_t>> next_;
};

struct LIBIME_EXPORT PinyinSyllable {
public:
    PinyinSyllable(PinyinInitial initial, PinyinFinal final)
        : initial_(initial), final_(final) {}
    PinyinSyllable(const PinyinSyllable &) = default;

    PinyinInitial initial() const { return initial_; }
    PinyinFinal final() const { return final_; }

    std::string toString() const;

    PinyinSyllable &operator=(const PinyinSyllable &) = default;
    bool operator==(const PinyinSyllable &other) {
        return initial_ == other.initial_ && final_ == other.final_;
    }

private:
    PinyinInitial initial_;
    PinyinFinal final_;
};

class LIBIME_EXPORT PinyinEncoder {
public:
    static PinyinSegments parseUserPinyin(boost::string_view pinyin,
                                          PinyinFuzzyFlags flags);

    static std::vector<char> encodeFullPinyin(boost::string_view pinyin);

    static std::string decodeFullPinyin(const std::vector<char> &v) {
        return decodeFullPinyin(v.data(), v.size());
    }
    static std::string decodeFullPinyin(const char *data, size_t size);

    static const std::string &initialToString(PinyinInitial initial);
    static PinyinInitial stringToInitial(const std::string &str);
    static bool isValidInitial(char c) {
        return c >= firstInitial && c <= lastInitial;
    }

    static const std::string &finalToString(PinyinFinal final);
    static PinyinFinal stringToFinal(const std::string &str);
    static bool isValidFinal(char c) {
        return c >= firstFinal && c <= lastFinal;
    }

    static bool isValidInitialFinal(PinyinInitial initial, PinyinFinal final);

    static std::vector<std::pair<PinyinInitial, std::vector<PinyinFinal>>>
    stringToSyllables(boost::string_view pinyin, PinyinFuzzyFlags flags);

    static const char initialFinalSepartor = '!';
    static const char firstInitial = static_cast<char>(PinyinInitial::B);
    static const char lastInitial = static_cast<char>(PinyinInitial::Zero);
    static const char firstFinal = static_cast<char>(PinyinFinal::A);
    static const char lastFinal = static_cast<char>(PinyinFinal::Zero);
};
}

#endif // _FCITX_LIBIME_PINYINENCODER_H_
