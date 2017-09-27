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
#ifndef _FCITX_LIBIME_CORE_CONSTANTS_H_
#define _FCITX_LIBIME_CORE_CONSTANTS_H_

namespace libime {

constexpr float DEFAULT_USER_LANGUAGE_MODEL_UNIGRAM_WEIGHT = 3;
constexpr float DEFAULT_USER_LANGUAGE_MODEL_BIGRAM_WEIGHT = 7;
constexpr float DEFAULT_USER_LANGUAGE_MODEL_PENALTY_FACTOR = 0.3;
constexpr float DEFAULT_LANGUAGE_MODEL_UNKNOWN_PROBABILITY_PENALTY =
    1 / 60000000.0f;
// -38... is log10(2^-127)
constexpr float MIN_FLOAT_LOG10 = -38.23080944932561;
constexpr float DEFAULT_USER_LANGUAGE_MODEL_USER_WEIGHT = 0.1f;
}

#endif // _FCITX_LIBIME_CORE_CONSTANTS_H_
