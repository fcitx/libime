/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/stream.hpp>
#include <fcitx-utils/log.h>
#include "libime/core/historybigram.h"
#include "libime/core/lattice.h"
#include "libime/core/userlanguagemodel.h"
#include "libime/pinyin/pinyincontext.h"
#include "libime/pinyin/pinyindecoder.h"
#include "libime/pinyin/pinyindictionary.h"
#include "libime/pinyin/pinyinencoder.h"
#include "libime/pinyin/pinyinime.h"
#include "libime/pinyin/shuangpinprofile.h"
#include "testdir.h"
#include "testutils.h"

using namespace libime;

int main(int argc, char *argv[]) {
    auto printTime = [](int64_t t) {
        std::cout << "Time: " << t / 1000000.0 << " ms" << std::endl;
    };
    fcitx::Log::setLogRule("libime=5");
    PinyinIME ime(
        std::make_unique<PinyinDictionary>(),
        std::make_unique<UserLanguageModel>(LIBIME_BINARY_DIR "/data/sc.lm"));
    ime.setNBest(2);
    ime.dict()->load(PinyinDictionary::SystemDict,
                     LIBIME_BINARY_DIR "/data/sc.dict",
                     PinyinDictFormat::Binary);
    if (argc >= 2) {
        ime.dict()->load(PinyinDictionary::UserDict, argv[1],
                         PinyinDictFormat::Binary);
    }
    if (argc >= 3) {
        std::fstream fin(argv[2], std::ios::in | std::ios::binary);
        ime.model()->history().load(fin);
    }
    ime.setFuzzyFlags({PinyinFuzzyFlag::Inner, PinyinFuzzyFlag::CommonTypo,
                       PinyinFuzzyFlag::AdvancedTypo});
    ime.setScoreFilter(1.0F);
    ime.setShuangpinProfile(
        std::make_shared<ShuangpinProfile>(ShuangpinBuiltinProfile::Xiaohe));
    PinyinContext c(&ime);

    std::string word;
    while (std::cin >> word) {
        bool printAll = false;
        ScopedNanoTimer t(printTime);
        if (word == "back") {
            c.backspace();
        } else if (word == "reset") {
            c.clear();
        } else if (word == "cancel") {
            c.cancel();
        } else if (word == "left") {
            if (c.cursor() > 0) {
                c.setCursor(c.cursor() - 1);
            }
        } else if (word == "right") {
            if (c.cursor() < c.size()) {
                c.setCursor(c.cursor() + 1);
            }
        } else if (word.size() == 1 &&
                   (('a' <= word[0] && word[0] <= 'z') ||
                    (!c.userInput().empty() && word[0] == '\''))) {
            c.type(word);
        } else if (word.size() == 1 && ('0' <= word[0] && word[0] <= '9')) {
            size_t idx;
            if (word[0] == '0') {
                idx = 9;
            } else {
                idx = word[0] - '1';
            }
            if (c.candidates().size() >= idx) {
                c.select(idx);
            }
        } else if (word == "all") {
            printAll = true;
        } else if (word == "quit") {
            break;
        }
        if (c.selected()) {
            std::cout << "COMMIT:   " << c.preedit() << std::endl;
            c.learn();
            c.clear();
            continue;
        }
        std::cout << "PREEDIT:  " << c.preedit() << std::endl;
        std::cout << "SENTENCE: " << c.sentence() << std::endl;
        size_t count = 1;
        for (const auto &candidate : c.candidatesToCursor()) {
            std::cout << (count % 10) << ": ";
            for (const auto *node : candidate.sentence()) {
                const auto &pinyin =
                    node->as<PinyinLatticeNode>().encodedPinyin();
                std::cout << node->word();
                if (!pinyin.empty()) {
                    std::cout << " " << PinyinEncoder::decodeFullPinyin(pinyin);
                }
            }
            std::cout << " " << candidate.score() << std::endl;
            count++;
            if (!printAll && count > 10) {
                break;
            }
        }
    }

    boost::iostreams::stream<boost::iostreams::null_sink> nullOstream(
        (boost::iostreams::null_sink()));
    ime.dict()->save(PinyinDictionary::UserDict, nullOstream,
                     PinyinDictFormat::Binary);
    ime.model()->history().dump(nullOstream);

    return 0;
}
