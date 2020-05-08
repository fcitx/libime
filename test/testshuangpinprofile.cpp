/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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

void checkXiaoHe() {
    PinyinEncoder::parseUserShuangpin(
        "aiaaah", ShuangpinProfile(ShuangpinBuiltinProfile::Xiaohe),
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
}

void checkSegments(const libime::SegmentGraph &graph,
                   std::vector<std::vector<std::string>> segments) {
    graph.dfs([&segments](const SegmentGraphBase &segs,
                          const std::vector<size_t> &path) {
        size_t s = 0;
        std::vector<std::string> segment;
        for (auto e : path) {
            segment.push_back(std::string(segs.segment(s, e)));
            std::cout << segment.back() << " ";
            s = e;
        }
        segments.erase(std::remove(segments.begin(), segments.end(), segment),
                       segments.end());
        std::cout << std::endl;
        return true;
    });
    FCITX_ASSERT(segments.empty()) << "Remaining segments: " << segments;
}

int main() {
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::Ziranma));
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::MS));
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::ABC));
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::Ziguang));
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::Zhongwenzhixing));
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::PinyinJiajia));
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::Xiaohe));

    checkXiaoHe();

    std::vector<std::vector<std::string>> expectedSegments();
    // wo jiu shi xiang ce shi yi xia
    checkSegments(PinyinEncoder::parseUserShuangpin(
                      "wojquixdceuiyixw",
                      ShuangpinProfile(ShuangpinBuiltinProfile::MS),
                      PinyinFuzzyFlag::None),
                  {{"wo", "jq", "ui", "xd", "ce", "ui", "yi", "xw"}});

    checkSegments(PinyinEncoder::parseUserShuangpin(
                      "aaaierorilah",
                      ShuangpinProfile(ShuangpinBuiltinProfile::Ziranma),
                      PinyinFuzzyFlag::None),
                  {{"aa", "ai", "er", "or", "il", "ah"}});

    struct {
        const char *qp;
        const char *sp;
    } zrmZero[] = {
        {"a", "aa"},   {"ai", "ai"}, {"an", "an"}, {"ang", "ah"},
        {"ao", "ao"},  {"e", "ee"},  {"ei", "ei"}, {"en", "en"},
        {"eng", "eg"}, {"er", "er"}, {"o", "oo"},  {"ou", "ou"},
    };
    ShuangpinProfile zrm(ShuangpinBuiltinProfile::Ziranma);
    for (auto [qp, sp] : zrmZero) {
        auto matchedSp =
            PinyinEncoder::shuangpinToSyllables(sp, zrm, PinyinFuzzyFlag::None);
        auto matchedQp =
            PinyinEncoder::stringToSyllables(qp, PinyinFuzzyFlag::None);
        FCITX_ASSERT(matchedQp == matchedSp)
            << " " << matchedQp << " " << matchedSp;
    }

    std::string zrmText = "[方案]\n"
                          "方案名称=自定义\n"
                          "\n"
                          "[零声母标识]\n"
                          "=O*\n"
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
