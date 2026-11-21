/*
 * SPDX-FileCopyrightText: 2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_PINYIN_PINYINENCODER_P_H_
#define _FCITX_LIBIME_PINYIN_PINYINENCODER_P_H_

#include <algorithm>
#include <cstddef>
#include <span>
#include <libime/pinyin/pinyinencoder.h>

namespace libime {

// Expand a normalized syllable using only explicitly enabled fuzzy pinyin
// rules. Input-level ambiguity rules are not applied.
MatchedPinyinSyllablesWithFuzzyFlags
expandSyllableWithExplicitFuzzy(PinyinSyllable syllable,
                                PinyinFuzzyFlags flags);

} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINENCODER_P_H_
