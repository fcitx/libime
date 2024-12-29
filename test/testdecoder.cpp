/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "libime/core/decoder.h"
#include "libime/core/languagemodel.h"
#include "libime/core/lattice.h"
#include "libime/core/segmentgraph.h"
#include "libime/pinyin/pinyindecoder.h"
#include "libime/pinyin/pinyindictionary.h"
#include "libime/pinyin/pinyinencoder.h"
#include "testdir.h"
#include "testutils.h"
#include <cstddef>
#include <fcitx-utils/log.h>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>

using namespace libime;

void testTime(PinyinDictionary & /*unused*/, Decoder &decoder,
              const char *pinyin, PinyinFuzzyFlags flags, int nbest = 1) {
    auto printTime = [](int t) {
        std::cout << "Time: " << t / 1000000.0 << " ms" << std::endl;
    };
    ScopedNanoTimer timer(printTime);
    auto graph = PinyinEncoder::parseUserPinyin(pinyin, flags);
    Lattice lattice;
    decoder.decode(lattice, graph, nbest, decoder.model()->nullState(),
                   std::numeric_limits<float>::max(),
                   -std::numeric_limits<float>::max(), Decoder::beamSizeDefault,
                   Decoder::frameSizeDefault, nullptr);
    for (size_t i = 0, e = lattice.sentenceSize(); i < e; i++) {
        const auto &sentence = lattice.sentence(i);
        for (const auto &p : sentence.sentence()) {
            std::cout << p->word() << " ";
        }
        std::cout << sentence.score() << std::endl;
    }
}

int main() {
    PinyinDictionary dict;
    dict.load(PinyinDictionary::SystemDict, LIBIME_BINARY_DIR "/data/sc.dict",
              PinyinDictFormat::Binary);
    LanguageModel model(LIBIME_BINARY_DIR "/data/sc.lm");
    PinyinDecoder decoder(&dict, &model);
    testTime(dict, decoder, "wojiushixiangceshi", PinyinFuzzyFlag::None);
    testTime(dict, decoder, "xian", PinyinFuzzyFlag::Inner);
    testTime(dict, decoder, "xiian", PinyinFuzzyFlag::Inner);
    testTime(dict, decoder, "tanan", PinyinFuzzyFlag::Inner);
    testTime(dict, decoder, "jin'an", PinyinFuzzyFlag::Inner);
    testTime(dict, decoder, "sh'a", PinyinFuzzyFlag::Inner);
    testTime(dict, decoder, "xiian", PinyinFuzzyFlag::Inner);
    testTime(dict, decoder, "anqilaibufangbian", PinyinFuzzyFlag::Inner);
    testTime(dict, decoder, "zhizuoxujibianchengleshunshuituizhoudeshiqing",
             PinyinFuzzyFlag::Inner, 2);
    testTime(dict, decoder, "xi'ian", PinyinFuzzyFlag::Inner);
    testTime(dict, decoder, "zuishengmengsi'''", PinyinFuzzyFlag::Inner);
    testTime(dict, decoder, "yongtiechuichuidanchuibupo",
             PinyinFuzzyFlag::Inner);
    testTime(dict, decoder, "feibenkerenyuanbunengrunei",
             PinyinFuzzyFlag::Inner);
    testTime(dict, decoder, "feibenkerenyuanbuderunei", PinyinFuzzyFlag::Inner);
    testTime(dict, decoder, "yongtiechuichuidanchuibupo",
             PinyinFuzzyFlag::Inner, 2);
    testTime(dict, decoder, "feibenkerenyuanbuderunei", PinyinFuzzyFlag::Inner,
             2);
    testTime(dict, decoder, "tashiyigehaoren", PinyinFuzzyFlag::Inner, 3);
    testTime(dict, decoder, "xianshi", PinyinFuzzyFlag::Inner, 20);
    testTime(dict, decoder, "xianshi", PinyinFuzzyFlag::Inner, 1);
    testTime(dict, decoder, "'xianshi", PinyinFuzzyFlag::Inner, 1);
    testTime(dict, decoder, "zhuoyand", PinyinFuzzyFlag::Inner, 1);
    testTime(dict, decoder, "nd", PinyinFuzzyFlag::Inner, 1);
    testTime(dict, decoder, "zhzxjbchlshshtzhdshq", PinyinFuzzyFlag::Inner, 1);
    testTime(dict, decoder, "tashini", PinyinFuzzyFlag::Inner, 2);
    testTime(dict, decoder, "'''", PinyinFuzzyFlag::Inner, 2);
    // testTime(dict, decoder, "n", PinyinFuzzyFlag::Inner);

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
        dict.matchPrefix(graph, [](const SegmentGraphPath &, WordNode &, float,
                                   std::unique_ptr<LatticeNodeData>) {});
    }
    testTime(dict, decoder, "sdfsdfsdfsdfsdfsdfsdf", PinyinFuzzyFlag::None, 2);
    testTime(dict, decoder, "ceshiyixiayebuhuichucuo", PinyinFuzzyFlag::None,
             2);
    return 0;
}
