/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/historybigram.h"
#include <boost/range/irange.hpp>
#include <fcitx-utils/log.h>
#include <sstream>

void testBasic() {
    using namespace libime;
    HistoryBigram history;
    history.setUnknownPenalty(std::log10(1.0f / 8192));
    history.add({"你", "是", "一个", "好人"});
    history.add({"我", "是", "一个", "坏人"});
    history.add({"他"});

    history.dump(std::cout);
    std::cout << history.score("你", "是") << std::endl;
    std::cout << history.score("他", "是") << std::endl;
    std::cout << history.score("他", "不是") << std::endl;
    {
        std::stringstream ss;
        history.save(ss);
        history.clear();
        std::cout << history.score("你", "是") << std::endl;
        std::cout << history.score("他", "是") << std::endl;
        std::cout << history.score("他", "不是") << std::endl;
        history.load(ss);
    }
    std::cout << history.score("你", "是") << std::endl;
    std::cout << history.score("他", "是") << std::endl;
    std::cout << history.score("他", "不是") << std::endl;

    history.dump(std::cout);
    {
        std::stringstream ss;
        try {
            history.load(ss);
        } catch (const std::exception &e) {
            history.clear();
        }
    }
    std::cout << history.score("你", "是") << std::endl;
    std::cout << history.score("他", "是") << std::endl;
    std::cout << history.score("他", "不是") << std::endl;

    std::cout << "--------------------" << std::endl;
    history.clear();
    history.add({"泥浩"});
    std::cout << history.score("", "泥浩") << " " << history.score("", "你好")
              << std::endl;
    history.add({"你好"});
    std::cout << history.score("", "泥浩") << " " << history.score("", "你好")
              << std::endl;
    for (int i = 0; i < 40; i++) {
        history.add({"泥浩"});
        std::cout << history.score("", "泥浩") << " "
                  << history.score("", "你好") << std::endl;
    }
    history.add({"你好"});
    std::cout << history.score("", "泥浩") << " " << history.score("", "你好")
              << std::endl;
    history.add({"你好"});
    std::cout << history.score("", "泥浩") << " " << history.score("", "你好")
              << std::endl;
    history.add({"泥浩"});
    std::cout << history.score("", "泥浩") << " " << history.score("", "你好")
              << std::endl;
    history.add({"泥浩"});
    std::cout << history.score("", "泥浩") << " " << history.score("", "你好")
              << std::endl;

    history.clear();
    for (int i = 0; i < 1; i++) {
        history.add({"跑", "不", "起来"});
        std::cout << "paobuqilai "
                  << history.score("", "跑") + history.score("跑", "不") +
                         history.score("不", "起来")
                  << " "
                  << history.score("", "跑步") + history.score("跑步", "起来")
                  << std::endl;
    }
    for (int i = 0; i < 100; i++) {
        history.add({"跑步", "起来"});
        std::cout << "paobuqilai "
                  << history.score("", "跑") + history.score("跑", "不") +
                         history.score("不", "起来")
                  << " "
                  << history.score("", "跑步") + history.score("跑步", "起来")
                  << std::endl;
    }
    FCITX_ASSERT(!history.isUnknown("跑步"));
    history.forget("跑步");
    FCITX_ASSERT(history.isUnknown("跑步"));
    std::cout << "paobuqilai "
              << history.score("", "跑") + history.score("跑", "不") +
                     history.score("不", "起来")
              << " "
              << history.score("", "跑步") + history.score("跑步", "起来")
              << std::endl;
}

void testOverflow() {
    using namespace libime;
    HistoryBigram history;
    constexpr auto total = 100000;
    for (auto i : boost::irange(0, total)) {
        history.add({std::to_string(i)});
    }
    std::stringstream dump;
    history.dump(dump);
    int i, expect = total - 1;
    while (dump >> i) {
        FCITX_ASSERT(i == expect);
        --expect;
    }
}

void testPredict() {
    using namespace libime;
    HistoryBigram history;
    constexpr auto total = 10000;
    for (auto i : boost::irange(0, total)) {
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

int main() {
    testBasic();
    testOverflow();
    testPredict();
    return 0;
}
