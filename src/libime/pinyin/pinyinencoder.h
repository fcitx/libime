/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_PINYIN_PINYINENCODER_H_
#define _FCITX_LIBIME_PINYIN_PINYINENCODER_H_

#include "libimepinyin_export.h"
#include <cassert>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/log.h>
#include <functional>
#include <libime/core/segmentgraph.h>
#include <string>
#include <string_view>
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
    InnerShort = 1 << 15,
    PartialFinal = 1 << 16,
    /**
     * Enable matching partial shuangpin
     *
     * @since 1.0.10
     */
    PartialSp = 1 << 17,
};

using PinyinFuzzyFlags = fcitx::Flags<PinyinFuzzyFlag>;

LIBIMEPINYIN_EXPORT
fcitx::LogMessageBuilder &operator<<(fcitx::LogMessageBuilder &log,
                                     PinyinFuzzyFlags final);

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

LIBIMEPINYIN_EXPORT
fcitx::LogMessageBuilder &operator<<(fcitx::LogMessageBuilder &log,
                                     PinyinInitial initial);

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

LIBIMEPINYIN_EXPORT
fcitx::LogMessageBuilder &operator<<(fcitx::LogMessageBuilder &log,
                                     PinyinFinal final);

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

LIBIMEPINYIN_EXPORT
fcitx::LogMessageBuilder &operator<<(fcitx::LogMessageBuilder &log,
                                     PinyinSyllable syl);

using MatchedPinyinSyllables = std::vector<
    std::pair<PinyinInitial, std::vector<std::pair<PinyinFinal, bool>>>>;

class LIBIMEPINYIN_EXPORT PinyinEncoder {
public:
    static SegmentGraph parseUserPinyin(std::string pinyin,
                                        PinyinFuzzyFlags flags);
    static SegmentGraph parseUserShuangpin(std::string pinyin,
                                           const ShuangpinProfile &sp,
                                           PinyinFuzzyFlags flags);

    /**
     * @brief Encode a quote separated pinyin string.
     *
     * @param pinyin pinyin string, like ni'hao
     * @return encoded pinyin.
     */
    static std::vector<char> encodeFullPinyin(std::string_view pinyin);
    static std::vector<char> encodeOneUserPinyin(std::string pinyin);

    static std::string shuangpinToPinyin(std::string_view pinyin,
                                         const ShuangpinProfile &sp);

    static bool isValidUserPinyin(const char *data, size_t size);

    static bool isValidUserPinyin(const std::vector<char> &v) {
        return isValidUserPinyin(v.data(), v.size());
    }

    static std::string decodeFullPinyin(const std::vector<char> &v) {
        return decodeFullPinyin(v.data(), v.size());
    }
    static std::string decodeFullPinyin(std::string_view s) {
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
    // This will use "ü" when possible.
    static std::string initialFinalToPinyinString(PinyinInitial initial,
                                                  PinyinFinal final);

    static MatchedPinyinSyllables stringToSyllables(std::string_view pinyin,
                                                    PinyinFuzzyFlags flags);
    static MatchedPinyinSyllables
    shuangpinToSyllables(std::string_view pinyin, const ShuangpinProfile &sp,
                         PinyinFuzzyFlags flags);

    static const char firstInitial = static_cast<char>(PinyinInitial::B);
    static const char lastInitial = static_cast<char>(PinyinInitial::Zero);
    static const char firstFinal = static_cast<char>(PinyinFinal::A);
    static const char lastFinal = static_cast<char>(PinyinFinal::Zero);
};
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINENCODER_H_
