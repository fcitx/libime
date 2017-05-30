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
#include "libime/pinyindata.h"
#include "libime/shuangpinprofile.h"
#include <iostream>
using namespace libime;

void checkProfile(const ShuangpinProfile &profile) {

    for (auto &p : profile.table()) {
        if (p.second.size() >= 2) {
            std::cout << p.first;
            for (auto &py : p.second) {
                std::cout << " " << py.first.toString() << " "
                          << static_cast<int>(py.second);
            }
            std::cout << std::endl;
        }
    }

    std::set<PinyinSyllable> validSyls;
    for (auto &p : getPinyinMap()) {
        validSyls.emplace(p.initial(), p.final());
    }
    validSyls.erase(PinyinSyllable(PinyinInitial::M, PinyinFinal::Zero));
    validSyls.erase(PinyinSyllable(PinyinInitial::N, PinyinFinal::Zero));
    validSyls.erase(PinyinSyllable(PinyinInitial::R, PinyinFinal::Zero));

    for (auto &p : profile.table()) {
        for (auto &py : p.second) {
            // check coverage
            if (py.second == PinyinFuzzyFlag::None) {
                validSyls.erase(py.first);
            }
        }
    }

    std::cout << "not covered" << std::endl;
    for (auto &syl : validSyls) {
        std::cout << syl.toString() << std::endl;
    }
    assert(validSyls.size() == 0);
}

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
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::Ziranma));
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::MS));
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::ABC));
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::Ziguang));
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::Zhongwenzhixing));
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::PinyinJiajia));
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::Xiaohe));

    // wo jiu shi xiang ce shi yi xia
    PinyinEncoder::parseUserShuangpin(
        "wojquixdceuiyixw", ShuangpinProfile(ShuangpinBuiltinProfile::MS),
        PinyinFuzzyFlag::None)
        .dfs(callback);

    return 0;
}
