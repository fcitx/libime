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

#include "libime/lattice.h"
#include "libime/pinyinencoder.h"
#include <iostream>

using namespace libime;

bool callback(const SegmentGraph &segs, const std::vector<size_t> &path) {
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
    PinyinEncoder::parseUserPinyin("jiaou", PinyinFuzzyFlag::Inner)
        .dfs(callback);
    PinyinEncoder::parseUserPinyin("jin'an", PinyinFuzzyFlag::Inner)
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
    {
        // xiang o n
        PinyinEncoder::parseUserPinyin("xian", PinyinFuzzyFlag::None)
            .dfs(callback);

        // xian gong
        PinyinEncoder::parseUserPinyin("xiangong", PinyinFuzzyFlag::None)
            .dfs(callback);

        // xiang o n
        PinyinEncoder::parseUserPinyin("xiangon", PinyinFuzzyFlag::None)
            .dfs(callback);

        // yan d
        PinyinEncoder::parseUserPinyin("yand", PinyinFuzzyFlag::None)
            .dfs(callback);
        // hua c o
        PinyinEncoder::parseUserPinyin("huaco", PinyinFuzzyFlag::None)
            .dfs(callback);
        // hua c o
        PinyinEncoder::parseUserPinyin("xion", PinyinFuzzyFlag::None)
            .dfs(callback);
        PinyinEncoder::parseUserPinyin("xiana", PinyinFuzzyFlag::None)
            .dfs(callback);
    }

    {
        auto graph = PinyinEncoder::parseUserPinyin("", PinyinFuzzyFlag::None);
        {
            auto graph2 =
                PinyinEncoder::parseUserPinyin("z", PinyinFuzzyFlag::None);
            auto s = graph.check(graph2);
            Lattice l;
            graph.merge(graph2, s, l);
        }
        graph.dfs(callback);
        {
            auto graph2 =
                PinyinEncoder::parseUserPinyin("zn", PinyinFuzzyFlag::None);
            auto s = graph.check(graph2);
            Lattice l;
            graph.merge(graph2, s, l);
        }
        graph.dfs(callback);
        {
            auto graph2 =
                PinyinEncoder::parseUserPinyin("z", PinyinFuzzyFlag::None);
            auto s = graph.check(graph2);
            Lattice l;
            graph.merge(graph2, s, l);
        }
        graph.dfs(callback);
    }
    {
        auto result =
            PinyinEncoder::stringToSyllables("z", PinyinFuzzyFlag::None);
        for (auto p : result) {
            for (auto f : p.second) {
                std::cout << PinyinEncoder::initialToString(p.first)
                          << PinyinEncoder::finalToString(f) << std::endl;
            }
        }
    }
    return 0;
}
