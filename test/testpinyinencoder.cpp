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

void dfs(const PinyinSegments &segs, std::vector<size_t> &path, size_t start = 0) {
    if (start == segs.end()) {
        size_t s = 0;
        for (auto e : path) {
            std::cout << segs.segment(s, e) << " ";
            s = e;
        }
        std::cout << std::endl;
        return;
    }
    auto &nexts = segs.next(start);
    for (auto next : nexts) {
        path.push_back(next);
        dfs(segs, path, next);
        path.pop_back();
    }
}

int main() {
    std::vector<size_t> path;
    dfs(PinyinEncoder::parseUserPinyin("lvenu", PinyinFuzzyFlag::None), path);
    dfs(PinyinEncoder::parseUserPinyin("woaizuguotiananmen", PinyinFuzzyFlag::None), path);
    dfs(PinyinEncoder::parseUserPinyin("wanan", PinyinFuzzyFlag::None), path);
    dfs(PinyinEncoder::parseUserPinyin("biiiiiilp", PinyinFuzzyFlag::None), path);
    dfs(PinyinEncoder::parseUserPinyin("zhm", PinyinFuzzyFlag::None), path);
    dfs(PinyinEncoder::parseUserPinyin("zzhzhhzhzh", PinyinFuzzyFlag::None), path);
    dfs(PinyinEncoder::parseUserPinyin("shuou", PinyinFuzzyFlag::None), path);
    dfs(PinyinEncoder::parseUserPinyin("tanan", PinyinFuzzyFlag::None), path);
    dfs(PinyinEncoder::parseUserPinyin("lven", PinyinFuzzyFlag::None), path);
    dfs(PinyinEncoder::parseUserPinyin("ananananana", PinyinFuzzyFlag::None), path);
    return 0;
}
