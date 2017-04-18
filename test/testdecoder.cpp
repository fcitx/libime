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
#include "decoder.h"
#include "languagemodel.h"
#include "pinyindictionary.h"
#include "pinyinencoder.h"
#include <chrono>
#include <iostream>

struct ScopedNanoTimer {
    std::chrono::high_resolution_clock::time_point t0;
    std::function<void(int)> cb;

    ScopedNanoTimer(std::function<void(int)> callback)
        : t0(std::chrono::high_resolution_clock::now()), cb(callback) {}
    ~ScopedNanoTimer(void) {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto nanos =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0)
                .count();

        cb(nanos);
    }
};

using namespace libime;

void testTime(Decoder &decoder, const char *pinyin, PinyinFuzzyFlags flags) {
    auto printTime = [](int t) {
        std::cout << "Time: " << t / 1000000.0 << " ms" << std::endl;
    };
    ScopedNanoTimer timer(printTime);
    auto graph = PinyinEncoder::parseUserPinyin(pinyin, flags);
    decoder.decode(graph, 1);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        return 1;
    }
    PinyinDictionary dict(argv[1], PinyinDictFormat::Text);
    LanguageModel model(argv[2]);
    Decoder decoder(&dict, &model);
    decoder.setUnknownHandler(
        [](const SegmentGraph &graph,
           const std::vector<const SegmentGraphNode *> &path,
           boost::string_view entry, float &adjust) -> bool {
#if 0
        do {
            if (path.front() == &graph.start()) {
                break;
            }
            return false;
        } while(0);
#endif
            adjust += -100.0f;
            return true;
        });
    testTime(decoder, "wojiushixiangceshi", PinyinFuzzyFlag::None);
    testTime(decoder, "xian", PinyinFuzzyFlag::Inner);
    testTime(decoder, "xiian", PinyinFuzzyFlag::Inner);
    testTime(decoder, "tanan", PinyinFuzzyFlag::Inner);
    testTime(decoder, "jin'an", PinyinFuzzyFlag::Inner);
    testTime(decoder, "sh'a", PinyinFuzzyFlag::Inner);
    testTime(decoder, "xiian", PinyinFuzzyFlag::Inner);
    testTime(decoder, "anqilaibufangbian", PinyinFuzzyFlag::Inner);
    testTime(decoder, "zhizuoxujibianchengleshunshuituizhoudeshiqing",
             PinyinFuzzyFlag::Inner);
    testTime(decoder, "xi'ian", PinyinFuzzyFlag::Inner);
    testTime(decoder, "zuishengmengsi'''", PinyinFuzzyFlag::Inner);
    return 0;
}
