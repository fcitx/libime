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
#include "libime/pinyin/pinyindata.h"
#include "libime/pinyin/shuangpinprofile.h"
#include <fcitx-utils/log.h>
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
    FCITX_ASSERT(validSyls.size() == 0);
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
        .dfs([](const SegmentGraphBase &segs, const std::vector<size_t> &path) {
            size_t s = 0;
            for (auto e : path) {
                std::cout << segs.segment(s, e) << " ";
                s = e;
            }
            std::cout << std::endl;
            return true;
        });

    ShuangpinProfile zrm(ShuangpinBuiltinProfile::Ziranma);

    std::string zrmText = "[方案]\n"
                          "方案名称=自定义\n"
                          "\n"
                          "[零声母标识]\n"
                          "=O\n"
                          "\n"
                          "[声母]\n"
                          "# 双拼编码就是它本身的声母不必列出\n"
                          "ch=I\n"
                          "sh=U\n"
                          "zh=V\n"
                          "\n"
                          "[韵母]\n"
                          "# 双拼编码就是它本身的韵母不必列出\n"
                          "ai=L\n"
                          "an=J\n"
                          "ang=H\n"
                          "ao=K\n"
                          "ei=Z\n"
                          "en=F\n"
                          "eng=G\n"
                          "er=R\n"
                          "ia=W\n"
                          "ian=M\n"
                          "iang=D\n"
                          "iao=C\n"
                          "ie=X\n"
                          "in=N\n"
                          "ing=Y\n"
                          "iong=S\n"
                          "iu=Q\n"
                          "ng=G\n"
                          "ong=S\n"
                          "ou=B\n"
                          "ua=W\n"
                          "uai=Y\n"
                          "uan=R\n"
                          "uang=D\n"
                          "ue=T\n"
                          "ui=V\n"
                          "un=P\n"
                          "ve=T\n"
                          "uo=O\n";
    std::stringstream ss(zrmText);
    ShuangpinProfile profile(ss);
    FCITX_ASSERT(profile.table() == zrm.table());
    FCITX_ASSERT(profile.validInput() == zrm.validInput());

    return 0;
}
