/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_PINYIN_PINYINDATA_P_H_
#define _FCITX_LIBIME_PINYIN_PINYINDATA_P_H_

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "libime/core/utils_p.h"

namespace libime {

using InnerSegmentMap =
    std::unordered_map<std::string,
                       std::vector<std::pair<std::string, std::string>>,
                       StringHash, std::equal_to<>>;

const std::unordered_map<std::string,
                         std::vector<std::pair<std::string, std::string>>,
                         StringHash, std::equal_to<>> &
getInnerSegmentV2();
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINDATA_P_H_
