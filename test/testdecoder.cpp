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
#include "libime/languagemodel.h"
#include "libime/pinyindecoder.h"
#include "libime/pinyindictionary.h"
#include "libime/pinyinencoder.h"
#include "testutils.h"
#include <iostream>

using namespace libime;

void testTime(Decoder &decoder, const char *pinyin, PinyinFuzzyFlags flags,
              int nbest = 1) {
    auto printTime = [](int t) {
        std::cout << "Time: " << t / 1000000.0 << " ms" << std::endl;
    };
    ScopedNanoTimer timer(printTime);
    auto graph = PinyinEncoder::parseUserPinyin(pinyin, flags);
    Lattice lattice;
    decoder.decode(lattice, graph, nbest, decoder.model()->nullState());
    for (size_t i = 0, e = lattice.sentenceSize(); i < e; i++) {
        auto &sentence = lattice.sentence(i);
        for (auto &p : sentence.sentence()) {
            std::cout << p->word() << " ";
        }
        std::cout << sentence.score() << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        return 1;
    }
    PinyinDictionary dict;
    dict.load(PinyinDictionary::SystemDict, argv[1], PinyinDictFormat::Binary);
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
    testTime(decoder, "zhuoyand", PinyinFuzzyFlag::Inner, 1);
    testTime(decoder, "nd", PinyinFuzzyFlag::Inner, 1);
    testTime(decoder, "zhzxjbchlshshtzhdshq", PinyinFuzzyFlag::Inner, 1);
    testTime(decoder, "tashini", PinyinFuzzyFlag::Inner, 2);
    testTime(decoder, "'''", PinyinFuzzyFlag::Inner, 2);
    // testTime(decoder, "n", PinyinFuzzyFlag::Inner);

    auto printTime = [](int t) {
        std::cout << "Time: " << t / 1000000.0 << " ms" << std::endl;
    };

    SegmentGraph graph;
    {
        ScopedNanoTimer timer(printTime);
        std::cout << "Parse Pinyin ";
        graph = PinyinEncoder::parseUserPinyin("sdfsdfsdfsdfsdfsdfsdf",
                                               PinyinFuzzyFlag::None);
    }
    {
        // try do nothing
        ScopedNanoTimer timer(printTime);
        std::cout << "Pure Match ";
        dict.matchPrefix(graph, [](const SegmentGraphPath &, boost::string_view,
                                   float, boost::string_view) {});
    }
    testTime(decoder, "sdfsdfsdfsdfsdfsdfsdf", PinyinFuzzyFlag::None, 2);
    return 0;
}
