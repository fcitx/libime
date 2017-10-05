/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _FCITX_LIBIME_PINYIN_PINYINENCODER_H_
#define _FCITX_LIBIME_PINYIN_PINYINENCODER_H_

#include "libimepinyin_export.h"
#include <boost/utility/string_view.hpp>
#include <cassert>
#include <fcitx-utils/flags.h>
#include <functional>
#include <libime/core/segmentgraph.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace libime {

class ShuangpinProfile;

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

inline bool operator<(PinyinInitial l, PinyinInitial r) {
    return static_cast<char>(l) < static_cast<char>(r);
}

inline bool operator<=(PinyinInitial l, PinyinInitial r) {
    return l < r || l == r;
}

inline bool operator>(PinyinInitial l, PinyinInitial r) { return !(l <= r); }

inline bool operator>=(PinyinInitial l, PinyinInitial r) { return !(l < r); }

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

inline bool operator<(PinyinFinal l, PinyinFinal r) {
    return static_cast<char>(l) < static_cast<char>(r);
}

inline bool operator<=(PinyinFinal l, PinyinFinal r) { return l < r || l == r; }

inline bool operator>(PinyinFinal l, PinyinFinal r) { return !(l <= r); }

inline bool operator>=(PinyinFinal l, PinyinFinal r) { return !(l < r); }

struct LIBIMEPINYIN_EXPORT PinyinSyllable {
public:
    PinyinSyllable(PinyinInitial initial, PinyinFinal final)
        : initial_(initial), final_(final) {}
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(PinyinSyllable)

    PinyinInitial initial() const { return initial_; }
    PinyinFinal final() const { return final_; }

    std::string toString() const;

    bool operator==(const PinyinSyllable &other) const {
        return initial_ == other.initial_ && final_ == other.final_;
    }

    bool operator!=(const PinyinSyllable &other) const {
        return !(*this == other);
    }
    bool operator<(const PinyinSyllable &other) const {
        return std::make_pair(initial_, final_) <
               std::make_pair(other.initial_, other.final_);
    }
    bool operator<=(const PinyinSyllable &other) const {
        return *this < other || *this == other;
    }
    bool operator>(const PinyinSyllable &other) const {
        return !(*this <= other);
    }
    bool operator>=(const PinyinSyllable &other) const {
        return !(*this < other);
    }

private:
    PinyinInitial initial_;
    PinyinFinal final_;
};

using MatchedPinyinSyllables = std::vector<
    std::pair<PinyinInitial, std::vector<std::pair<PinyinFinal, bool>>>>;

class LIBIMEPINYIN_EXPORT PinyinEncoder {
public:
    static SegmentGraph parseUserPinyin(boost::string_view pinyin,
                                        PinyinFuzzyFlags flags);
    static SegmentGraph parseUserShuangpin(boost::string_view pinyin,
                                           const ShuangpinProfile &sp,
                                           PinyinFuzzyFlags flags);

    static std::vector<char> encodeFullPinyin(boost::string_view pinyin);
    static std::vector<char> encodeOneUserPinyin(boost::string_view pinyin);

    static std::string decodeFullPinyin(const std::vector<char> &v) {
        return decodeFullPinyin(v.data(), v.size());
    }
    static std::string decodeFullPinyin(boost::string_view s) {
        return decodeFullPinyin(s.data(), s.size());
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

    static MatchedPinyinSyllables stringToSyllables(boost::string_view pinyin,
                                                    PinyinFuzzyFlags flags);
    static MatchedPinyinSyllables
    shuangpinToSyllables(boost::string_view pinyin, const ShuangpinProfile &sp,
                         PinyinFuzzyFlags flags);

    static const char firstInitial = static_cast<char>(PinyinInitial::B);
    static const char lastInitial = static_cast<char>(PinyinInitial::Zero);
    static const char firstFinal = static_cast<char>(PinyinFinal::A);
    static const char lastFinal = static_cast<char>(PinyinFinal::Zero);
};
}

#endif // _FCITX_LIBIME_PINYIN_PINYINENCODER_H_
