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
#include "languagemodel.h"
#include "pinyindecoder.h"
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

void testTime(Decoder &decoder, const char *pinyin, PinyinFuzzyFlags flags,
              int nbest = 1) {
    auto printTime = [](int t) {
        std::cout << "Time: " << t / 1000000.0 << " ms" << std::endl;
    };
    ScopedNanoTimer timer(printTime);
    auto graph = PinyinEncoder::parseUserPinyin(pinyin, flags);
    auto lattice = decoder.decode(graph, nbest);
    for (size_t i = 0, e = lattice.sentenceSize(); i < e; i++) {
        auto &sentence = lattice.sentence(i);
        for (auto &p : sentence.sentence()) {
            std::cout << p.second << " ";
        }
        std::cout << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        return 1;
    }
    PinyinDictionary dict(argv[1], PinyinDictFormat::Binary);
    LanguageModel model(argv[2]);
    PinyinDecoder decoder(&dict, &model);
    testTime(decoder, "wojiushixiangceshi", PinyinFuzzyFlag::None);
    testTime(decoder, "xian", PinyinFuzzyFlag::Inner);
    testTime(decoder, "xiian", PinyinFuzzyFlag::Inner);
    testTime(decoder, "tanan", PinyinFuzzyFlag::Inner);
    testTime(decoder, "jin'an", PinyinFuzzyFlag::Inner);
    testTime(decoder, "sh'a", PinyinFuzzyFlag::Inner);
    testTime(decoder, "xiian", PinyinFuzzyFlag::Inner);
    testTime(decoder, "anqilaibufangbian", PinyinFuzzyFlag::Inner);
    testTime(decoder, "zhizuoxujibianchengleshunshuituizhoudeshiqing",
             PinyinFuzzyFlag::Inner, 2);
    testTime(decoder, "xi'ian", PinyinFuzzyFlag::Inner);
    testTime(decoder, "zuishengmengsi'''", PinyinFuzzyFlag::Inner);
    testTime(decoder, "yongtiechuichuidanchuibupo", PinyinFuzzyFlag::Inner);
    testTime(decoder, "feibenkerenyuanbunengrunei", PinyinFuzzyFlag::Inner);
    testTime(decoder, "feibenkerenyuanbuderunei", PinyinFuzzyFlag::Inner);
    testTime(decoder, "yongtiechuichuidanchuibupo", PinyinFuzzyFlag::Inner, 2);
    testTime(decoder, "feibenkerenyuanbuderunei", PinyinFuzzyFlag::Inner, 2);
    testTime(decoder, "tashiyigehaoren", PinyinFuzzyFlag::Inner, 3);
    testTime(decoder, "xianshi", PinyinFuzzyFlag::Inner, 20);
    testTime(decoder, "xianshi", PinyinFuzzyFlag::Inner, 1);
    testTime(decoder, "'xianshi", PinyinFuzzyFlag::Inner, 1);
    return 0;
}
