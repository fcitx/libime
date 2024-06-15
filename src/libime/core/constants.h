/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_CONSTANTS_H_
#define _FCITX_LIBIME_CORE_CONSTANTS_H_

namespace libime {

constexpr float DEFAULT_USER_LANGUAGE_MODEL_UNIGRAM_WEIGHT = 3;
constexpr float DEFAULT_USER_LANGUAGE_MODEL_BIGRAM_WEIGHT = 15;
constexpr float DEFAULT_LANGUAGE_MODEL_UNKNOWN_PROBABILITY_PENALTY =
    1 / 60000000.0F;
// -38... is log10(2^-127)
constexpr float HISTORY_BIGRAM_ALPHA_VALUE = 1.0F;
constexpr float MIN_FLOAT_LOG10 = -38.23080944932561;
constexpr float DEFAULT_USER_LANGUAGE_MODEL_USER_WEIGHT = 0.2F;
} // namespace libime

#endif // _FCITX_LIBIME_CORE_CONSTANTS_H_
