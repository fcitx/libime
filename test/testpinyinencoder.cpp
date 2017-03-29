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

#include "libime/pinyinencoder.h"
#include <iostream>

using namespace libime;

bool callback(const PinyinSegments &segs, const std::vector<size_t> &path) {
    size_t s = 0;
    for (auto e : path) {
        std::cout << segs.segment(s, e) << " ";
        s = e;
    }
    std::cout << std::endl;
    return true;
}

int main() {
    PinyinEncoder::parseUserPinyin("wa'nan'''", PinyinFuzzyFlag::None)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("lvenu", PinyinFuzzyFlag::None)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("woaizuguotiananmen", PinyinFuzzyFlag::None)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("wanan", PinyinFuzzyFlag::None)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("biiiiiilp", PinyinFuzzyFlag::None)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("zhm", PinyinFuzzyFlag::None).dfs(callback);
    PinyinEncoder::parseUserPinyin("zzhzhhzhzh", PinyinFuzzyFlag::None)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("shuou", PinyinFuzzyFlag::None)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("tanan", PinyinFuzzyFlag::None)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("lven", PinyinFuzzyFlag::None).dfs(callback);
    PinyinEncoder::parseUserPinyin("ananananana", PinyinFuzzyFlag::None)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("wa'nan", PinyinFuzzyFlag::None)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("xian", PinyinFuzzyFlag::None).dfs(callback);
    PinyinEncoder::parseUserPinyin("xian", PinyinFuzzyFlag::Inner)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("xi'an", PinyinFuzzyFlag::Inner)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("kuai", PinyinFuzzyFlag::None).dfs(callback);
    PinyinEncoder::parseUserPinyin("kuai", PinyinFuzzyFlag::Inner)
        .dfs(callback);

    for (auto syl : PinyinEncoder::stringToSyllables(
             "niagn",
             PinyinFuzzyFlags{PinyinFuzzyFlag::L_N, PinyinFuzzyFlag::IAN_IANG,
                              PinyinFuzzyFlag::NG_GN})) {
        for (auto f : syl.second) {
            std::cout << PinyinSyllable(syl.first, f).toString() << std::endl;
        }
    }
    for (auto syl : PinyinEncoder::stringToSyllables(
             "n",
             PinyinFuzzyFlags{PinyinFuzzyFlag::L_N, PinyinFuzzyFlag::IAN_IANG,
                              PinyinFuzzyFlag::NG_GN})) {
        for (auto f : syl.second) {
            std::cout << PinyinSyllable(syl.first, f).toString() << std::endl;
        }
    }
    for (auto syl : PinyinEncoder::stringToSyllables(
             "cuagn", {PinyinFuzzyFlag::C_CH, PinyinFuzzyFlag::UAN_UANG,
                       PinyinFuzzyFlag::NG_GN})) {
        for (auto f : syl.second) {
            std::cout << PinyinSyllable(syl.first, f).toString() << std::endl;
        }
    }
    return 0;
}
