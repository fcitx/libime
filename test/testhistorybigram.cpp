/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <cmath>
#include <exception>
#include <iostream>
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_set>
#include <fcitx-utils/log.h>
#include <fcitx-utils/stringutils.h>
#include "libime/core/historybigram.h"

namespace {

void testBasic() {
    using namespace libime;
    HistoryBigram history;
    history.setUnknownPenalty(std::log10(1.0F / 8192));
    history.add({"你", "是", "一个", "好人"});
    history.add({"我", "是", "一个", "坏人"});
    history.add({"他"});

    history.dump(std::cout);
    std::cout << history.score("你", "是") << '\n';
    std::cout << history.score("他", "是") << '\n';
    std::cout << history.score("他", "不是") << '\n';
    {
        std::stringstream ss;
        history.save(ss);
        history.clear();
        std::cout << history.score("你", "是") << '\n';
        std::cout << history.score("他", "是") << '\n';
        std::cout << history.score("他", "不是") << '\n';
        history.load(ss);
    }
    std::cout << history.score("你", "是") << '\n';
    std::cout << history.score("他", "是") << '\n';
    std::cout << history.score("他", "不是") << '\n';

    history.dump(std::cout);
    {
        std::stringstream ss;
        try {
            history.load(ss);
        } catch (const std::exception &e) {
            history.clear();
        }
    }
    std::cout << history.score("你", "是") << '\n';
    std::cout << history.score("他", "是") << '\n';
    std::cout << history.score("他", "不是") << '\n';

    std::cout << "--------------------" << '\n';
    history.clear();
    history.add({"泥浩"});
    std::cout << history.score("", "泥浩") << " " << history.score("", "你好")
              << '\n';
    history.add({"你好"});
    std::cout << history.score("", "泥浩") << " " << history.score("", "你好")
              << '\n';
    for (int i = 0; i < 40; i++) {
        history.add({"泥浩"});
        std::cout << history.score("", "泥浩") << " "
                  << history.score("", "你好") << '\n';
    }
    history.add({"你好"});
    std::cout << history.score("", "泥浩") << " " << history.score("", "你好")
              << '\n';
    history.add({"你好"});
    std::cout << history.score("", "泥浩") << " " << history.score("", "你好")
              << '\n';
    history.add({"泥浩"});
    std::cout << history.score("", "泥浩") << " " << history.score("", "你好")
              << '\n';
    history.add({"泥浩"});
    std::cout << history.score("", "泥浩") << " " << history.score("", "你好")
              << '\n';

    history.clear();
    for (int i = 0; i < 1; i++) {
        history.add({"跑", "不", "起来"});
        std::cout << "paobuqilai "
                  << history.score("", "跑") + history.score("跑", "不") +
                         history.score("不", "起来")
                  << " "
                  << history.score("", "跑步") + history.score("跑步", "起来")
                  << '\n';
    }
    for (int i = 0; i < 100; i++) {
        history.add({"跑步", "起来"});
        std::cout << "paobuqilai "
                  << history.score("", "跑") + history.score("跑", "不") +
                         history.score("不", "起来")
                  << " "
                  << history.score("", "跑步") + history.score("跑步", "起来")
                  << '\n';
    }
    FCITX_ASSERT(!history.isUnknown("跑步"));
    history.forget("跑步");
    FCITX_ASSERT(history.isUnknown("跑步"));
    std::cout << "paobuqilai "
              << history.score("", "跑") + history.score("跑", "不") +
                     history.score("不", "起来")
              << " "
              << history.score("", "跑步") + history.score("跑步", "起来")
              << '\n';
}

void testOverflow() {
    using namespace libime;
    HistoryBigram history;
    constexpr auto total = 100000;
    for (auto i : std::views::iota(0, total)) {
        history.add({std::to_string(i)});
    }
    std::stringstream dump;
    history.dump(dump);
    int i;
    int expect = total - 1;
    while (dump >> i) {
        FCITX_ASSERT(i == expect);
        --expect;
    }
}

void testPredict() {
    using namespace libime;
    HistoryBigram history;
    constexpr auto total = 10000;
    for (auto i : std::views::iota(0, total)) {
        history.add({std::to_string(i), std::to_string(i + 1)});
    }

    {
        std::unordered_set<std::string> result;
        history.fillPredict(result, {std::to_string(5)}, 10);
        FCITX_ASSERT(result ==
                     std::unordered_set<std::string>{std::to_string(6)});
    }

    {
        std::unordered_set<std::string> result;
        history.add({std::to_string(5), std::to_string(7)});
        history.add({std::to_string(5), std::to_string(4)});
        history.add({std::to_string(5), std::to_string(3)});
        history.add({std::to_string(5), std::to_string(6)});
        history.fillPredict(result, {std::to_string(5)}, 3);
        FCITX_ASSERT(result.size() == 3) << result;
        result.clear();
        history.fillPredict(result, {std::to_string(5)}, 0);
        FCITX_ASSERT(result ==
                     std::unordered_set<std::string>{
                         std::to_string(6), std::to_string(7),
                         std::to_string(3), std::to_string(4)})
            << result;
    }
}

void testSaveAndLoad() {
    using namespace libime;
    HistoryBigram history;
    history.add({std::to_string(1)});
    history.add({std::to_string(2)});
    history.add({std::to_string(3)});

    std::stringstream dump1;
    std::stringstream dump2;
    std::stringstream ss;
    history.save(ss);

    HistoryBigram history2;
    history2.load(ss);

    history.dump(dump1);
    history2.dump(dump2);

    FCITX_ASSERT(dump1.str() == dump2.str());
}

void testSaveAndLoadText() {
    using namespace libime;
    HistoryBigram history;
    constexpr auto total = 100000;
    for (auto i : std::views::iota(0, total)) {
        history.add({std::to_string(i)});
    }
    std::stringstream dump;
    history.dump(dump);
    int i;
    int expect = total - 1;
    while (dump >> i) {
        FCITX_ASSERT(i == expect);
        --expect;
    }

    std::stringstream dump1;
    std::stringstream dump2;
    std::stringstream ss;
    history.dump(ss);

    HistoryBigram history2;
    history2.loadText(ss);

    history.dump(dump1);
    history2.dump(dump2);

    FCITX_ASSERT(dump1.str() == dump2.str());
}

void testWithCode() {
    using namespace libime;
    HistoryBigram history;
    history.addWithCode({{"你", "code1"},
                         {"是", "code2"},
                         {"一个", "code3"},
                         {"好人", "code4"}});

    auto score = history.scoreWithCode({"你", "code1"}, {"是", "code2"});
    auto scoreWithoutCode = history.score("你", "是");
    auto scoreWithEmptyCode = history.scoreWithCode({"你", ""}, {"是", ""});
    auto scoreWithMismatchCode =
        history.scoreWithCode({"你", "code1"}, {"是", "code5"});
    FCITX_ASSERT(score == scoreWithoutCode) << score << " " << scoreWithoutCode;
    FCITX_ASSERT(score == scoreWithEmptyCode)
        << score << " " << scoreWithEmptyCode;
    FCITX_ASSERT(score > scoreWithMismatchCode)
        << score << " " << scoreWithMismatchCode;
    FCITX_ASSERT(history.rawUnigramFrequency({"你", ""}) == 1);
    FCITX_ASSERT(history.rawUnigramFrequency({"你", "code1"}) == 1);
    FCITX_ASSERT(history.rawUnigramFrequency({"你", "code2"}) == 0);
    FCITX_ASSERT(history.rawBigramFrequency({"你", ""}, {"是", ""}) == 1);
    FCITX_ASSERT(history.rawBigramFrequency({"你", "code1"}, {"是", "code2"}) ==
                 1);
    FCITX_ASSERT(history.rawBigramFrequency({"你", "code2"}, {"是", "code2"}) ==
                 0);
    FCITX_ASSERT(history.rawBigramFrequency({"你", ""}, {"是", "code2"}) == 1);
    FCITX_ASSERT(history.rawBigramFrequency({"你", "code1"}, {"是", ""}) == 1);
}

void testWithCodePredict() {
    using namespace libime;
    HistoryBigram history;
    history.addWithCode({{"你", "code1"},
                         {"是", "code2"},
                         {"一个", "code3"},
                         {"好人", "code4"}});

    {
        std::unordered_set<std::string> result;
        history.fillPredict(result, {"你"}, 10);
        FCITX_ASSERT(result == std::unordered_set<std::string>{"是"});
    }

    {
        std::unordered_set<std::string> result;
        history.addWithCode({{"你", "code1"}, {"是", "code5"}});
        history.addWithCode({{"你", "code1"}, {"是", "code6"}});
        history.addWithCode({{"你", "code1"}, {"是", "code7"}});
        history.addWithCode({{"你", "code1"}, {"是", "code8"}});
        history.fillPredict(result, {"你"}, 0);
        FCITX_ASSERT(result == std::unordered_set<std::string>{"是"}) << result;
    }
}

void testAppend() {
    using namespace libime;
    HistoryBigram history;
    history.addWithCode({{"你", "code1"}, {"是", "code2"}, {"一个", "code3"}});

    history.addWithContext({{"是", "code2"}, {"一个", "code3"}},
                           {{"好人", "code4"}});

    history.addWithContext({{"不是", "code5"}}, {{"你的", "code6"}});
    std::stringstream ss;
    history.dump(ss);
    auto lines = fcitx::stringutils::split(ss.str(), "\n");
    FCITX_ASSERT(lines.size() == 2) << lines.size();
    FCITX_ASSERT(lines[0] == "你的\tcode6") << lines[0];
    FCITX_ASSERT(lines[1] == "你\tcode1 是\tcode2 一个\tcode3 好人\tcode4")
        << lines[1];
}

} // namespace

int main() {
    testBasic();
    testOverflow();
    testPredict();
    testSaveAndLoad();
    testSaveAndLoadText();
    testWithCode();
    testWithCodePredict();
    testAppend();
    return 0;
}
