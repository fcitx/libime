/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/pinyin/pinyinencoder.h"
#include <fcitx-utils/log.h>
#include <string>
#include <vector>

using namespace libime;

void dfs(const SegmentGraph &segs,
         const std::vector<std::string> &expectedMatch = {}) {
    FCITX_ASSERT(segs.checkGraph());

    bool checked = false;
    auto callback = [&checked,
                     &expectedMatch](const SegmentGraphBase &segs,
                                     const std::vector<size_t> &path) {
        size_t start = 0;
        std::vector<std::string> result;
        for (auto end : path) {
            result.push_back(std::string(segs.segment(start, end)));
            start = end;
        }
        if (!checked && !expectedMatch.empty()) {
            if (result == expectedMatch) {
                checked = true;
            }
        }
        FCITX_INFO() << result;
        return true;
    };

    segs.dfs(callback);
    FCITX_ASSERT(checked || expectedMatch.empty())
        << segs.data() << " " << expectedMatch;
}

void check(std::string py, PinyinFuzzyFlags flags,
           const std::vector<std::string> &expectedMatch) {
    dfs(PinyinEncoder::parseUserPinyin(std::move(py), flags), expectedMatch);
}

int main() {
    check("wa'nan'''", PinyinFuzzyFlag::None, {"wa", "'", "nan", "'''"});
    check("lvenu", PinyinFuzzyFlag::None, {"lve", "nu"});
    check("woaizuguotiananmen", PinyinFuzzyFlag::None,
          {"wo", "ai", "zu", "guo", "tian", "an", "men"});
    check("wanan", PinyinFuzzyFlag::None, {"wan", "an"});
    check("biiiiiilp", PinyinFuzzyFlag::None, {"bi", "iiiiilp"});
    check("zhm", PinyinFuzzyFlag::None, {"zh", "m"});
    check("zzhzhhzhzh", PinyinFuzzyFlag::None,
          {"z", "zh", "zh", "h", "zh", "zh"});
    check("shuou", PinyinFuzzyFlag::None, {"shu", "ou"});
    check("tanan", PinyinFuzzyFlag::None, {"tan", "an"});
    check("lven", PinyinFuzzyFlag::None, {"lv", "en"});
    check("ananananana", PinyinFuzzyFlag::None,
          {"an", "an", "an", "an", "an", "a"});
    check("wa'nan", PinyinFuzzyFlag::None, {"wa", "'", "nan"});
    check("xian", PinyinFuzzyFlag::None, {"xian"});
    check("xian", PinyinFuzzyFlag::Inner, {"xi", "an"});
    check("xi'an", PinyinFuzzyFlag::Inner, {"xi", "'", "an"});
    check("kuai", PinyinFuzzyFlag::None, {"kuai"});
    check("kuai", PinyinFuzzyFlag::Inner, {"ku", "ai"});
    check("jiaou", PinyinFuzzyFlag::Inner, {"jia", "ou"});
    check("jin'an", PinyinFuzzyFlag::Inner, {"jin", "'", "an"});
    check("qie", PinyinFuzzyFlag::Inner, {"qie"});
    check("qie", PinyinFuzzyFlag::InnerShort, {"qi", "e"});
    check("qi'e", PinyinFuzzyFlag::Inner, {"qi", "'", "e"});
    check("nng", PinyinFuzzyFlag::InnerShort, {"n", "ng"});

    for (const auto &syl : PinyinEncoder::stringToSyllables(
             "niagn",
             PinyinFuzzyFlags{PinyinFuzzyFlag::L_N, PinyinFuzzyFlag::IAN_IANG,
                              PinyinFuzzyFlag::CommonTypo})) {
        for (auto f : syl.second) {
            FCITX_INFO() << PinyinSyllable(syl.first, f.first).toString();
        }
    }
    for (const auto &syl : PinyinEncoder::stringToSyllables(
             "n",
             PinyinFuzzyFlags{PinyinFuzzyFlag::L_N, PinyinFuzzyFlag::IAN_IANG,
                              PinyinFuzzyFlag::CommonTypo})) {
        for (auto f : syl.second) {
            FCITX_INFO() << PinyinSyllable(syl.first, f.first).toString();
        }
    }
    for (const auto &syl : PinyinEncoder::stringToSyllables(
             "cuagn", {PinyinFuzzyFlag::C_CH, PinyinFuzzyFlag::UAN_UANG,
                       PinyinFuzzyFlag::CommonTypo})) {
        for (auto f : syl.second) {
            FCITX_INFO() << PinyinSyllable(syl.first, f.first).toString();
        }
    }

    for (const auto &syl : PinyinEncoder::stringToSyllables(
             "e", PinyinFuzzyFlags{PinyinFuzzyFlag::PartialFinal})) {
        for (auto f : syl.second) {
            FCITX_INFO() << PinyinSyllable(syl.first, f.first).toString();
        }
    }
    {
        // xiang o n
        check("xian", PinyinFuzzyFlag::None, {"xian"});

        // xian gong
        check("xiangong", PinyinFuzzyFlag::None, {"xian", "gong"});

        // xiang o n
        check("xiangon", PinyinFuzzyFlag::None, {"xiang", "o", "n"});

        // yan d
        check("yand", PinyinFuzzyFlag::None, {"yan", "d"});
        // hua c o
        check("huaco", PinyinFuzzyFlag::None, {"hua", "c", "o"});
        // hua c o
        check("xion", PinyinFuzzyFlag::None, {"xi", "o", "n"});
        check("xiana", PinyinFuzzyFlag::None, {"xian", "a"});

        check("Nihao", PinyinFuzzyFlag::None, {"Ni", "hao"});
    }

    {
        auto graph = PinyinEncoder::parseUserPinyin("", PinyinFuzzyFlag::None);
        {
            auto graph2 =
                PinyinEncoder::parseUserPinyin("z", PinyinFuzzyFlag::None);
            graph.merge(graph2);
        }
        dfs(graph, {"z"});
        {
            auto graph2 =
                PinyinEncoder::parseUserPinyin("zn", PinyinFuzzyFlag::None);
            graph.merge(graph2);
        }
        dfs(graph, {"z", "n"});
        {
            auto graph2 =
                PinyinEncoder::parseUserPinyin("z", PinyinFuzzyFlag::None);
            graph.merge(graph2);
        }
        dfs(graph, {"z"});
    }
    {
        auto result =
            PinyinEncoder::stringToSyllables("z", PinyinFuzzyFlag::None);
        for (const auto &p : result) {
            for (auto f : p.second) {
                FCITX_INFO() << PinyinEncoder::initialToString(p.first)
                             << PinyinEncoder::finalToString(f.first);
            }
        }
    }

    bool hasException = false;
    try {
        PinyinEncoder::encodeFullPinyin("lue");
    } catch (const std::invalid_argument &) {
        hasException = true;
    }
    FCITX_ASSERT(hasException);

    {
        auto result = PinyinEncoder::encodeFullPinyinWithFlags(
            "lue", PinyinFuzzyFlag::VE_UE);
        FCITX_ASSERT(PinyinEncoder::decodeFullPinyin(result) == "lve");
    }
    {
        auto result = PinyinEncoder::encodeOneUserPinyin("nihao");
        FCITX_ASSERT(PinyinEncoder::decodeFullPinyin(result) == "ni'hao");
    }
    {
        auto result = PinyinEncoder::encodeOneUserPinyin("xian");
        FCITX_ASSERT(PinyinEncoder::decodeFullPinyin(result) == "xian");
    }
    {
        auto result = PinyinEncoder::encodeOneUserPinyin("xi'an");
        FCITX_ASSERT(PinyinEncoder::decodeFullPinyin(result) == "xi'an");
    }
    {
        auto result = PinyinEncoder::encodeOneUserPinyin("nh");
        FCITX_ASSERT(PinyinEncoder::decodeFullPinyin(result) == "n'h");
    }
    {
        auto result = PinyinEncoder::encodeOneUserPinyin("nfi");
        FCITX_INFO() << PinyinEncoder::decodeFullPinyin(result);
    }
    {
        auto result = PinyinEncoder::initialFinalToPinyinString(
            libime::PinyinInitial::N, libime::PinyinFinal::VE);
        FCITX_ASSERT(result == "nüe");
    }
    {
        auto result = PinyinEncoder::initialFinalToPinyinString(
            libime::PinyinInitial::L, libime::PinyinFinal::V);
        FCITX_ASSERT(result == "lü");
    }

    check("zhunipingan", PinyinFuzzyFlag::Inner, {"zhu", "ni", "ping", "an"});
    check("zhunipingan", PinyinFuzzyFlag::Inner, {"zhu", "ni", "pin", "gan"});
    return 0;
}
