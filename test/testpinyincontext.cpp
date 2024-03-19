/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/historybigram.h"
#include "libime/core/lattice.h"
#include "libime/core/userlanguagemodel.h"
#include "libime/pinyin/pinyincontext.h"
#include "libime/pinyin/pinyindecoder.h"
#include "libime/pinyin/pinyindictionary.h"
#include "libime/pinyin/pinyinime.h"
#include "testdir.h"
#include <array>
#include <fcitx-utils/log.h>
#include <sstream>

using namespace libime;

int main() {
    PinyinIME ime(
        std::make_unique<PinyinDictionary>(),
        std::make_unique<UserLanguageModel>(LIBIME_BINARY_DIR "/data/sc.lm"));
    ime.model()->history().add({"字迹", "格子"});
    // add a manual dict
    std::stringstream ss;
    ss << "献世 xian'shi 0.0\n";
    ime.dict()->load(PinyinDictionary::SystemDict,
                     LIBIME_BINARY_DIR "/data/sc.dict",
                     PinyinDictFormat::Binary);
    ime.dict()->load(PinyinDictionary::UserDict, ss, PinyinDictFormat::Text);
    ime.dict()->addWord(1, "zi'ji'ge'zi", "自机各自");
    ime.setFuzzyFlags(PinyinFuzzyFlag::Inner);
    PinyinContext c(&ime);
    c.type("xianshi");

    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.select(40);
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.cancel();
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.type("shi'");
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    int i = 0;
    for (const auto &candidate : c.candidates()) {
        if (candidate.toString() == "西安市") {
            break;
        }
        i++;
    }
    c.select(i);
    FCITX_ASSERT(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.select(0);
    FCITX_ASSERT(c.selected()) << c.sentence() << " " << c.preedit();
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.clear();
    FCITX_ASSERT(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.type("zi'ji'ge'zi'");
    FCITX_ASSERT(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    i = 0;
    for (const auto &candidate : c.candidates()) {
        if (candidate.toString() == "子集") {
            break;
        }
        i++;
    }
    c.select(i);
    FCITX_ASSERT(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }

    std::cout << "--------------------------------" << std::endl;
    c.select(0);
    FCITX_ASSERT(c.selected()) << c.sentence() << " " << c.preedit();
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    c.clear();
    c.type("n");
    for (const auto &candidate : c.candidates()) {
        for (const auto *node : candidate.sentence()) {
            const auto &pinyin =
                static_cast<const PinyinLatticeNode *>(node)->encodedPinyin();
            std::cout << node->word();
            if (!pinyin.empty()) {
                std::cout << " " << PinyinEncoder::decodeFullPinyin(pinyin);
            }
        }
        std::cout << std::endl;
    }

    std::cout << "--------------------------------" << std::endl;
    {
        c.clear();
        c.type("nvedai");
        std::array<size_t, 7> expectedCursor = {0, 1, 3, 4, 6, 7, 8};
        for (size_t i = 0; i <= c.size(); i++) {
            c.setCursor(i);
            size_t actualCursor =
                c.preeditWithCursor(PinyinPreeditMode::Pinyin).second;
            FCITX_ASSERT(actualCursor == expectedCursor[i])
                << "Raw Cursor: " << i << " Cursor: " << actualCursor
                << " Expected: " << expectedCursor[i];
        }
        FCITX_ASSERT(c.preedit(PinyinPreeditMode::Pinyin) == "nüe dai");
    }

    {
        c.clear();
        c.type("xi'an");
        std::array<size_t, 6> expectedCursor = {0, 1, 2, 4, 6, 7};
        for (size_t i = 0; i <= c.size(); i++) {
            c.setCursor(i);
            size_t actualCursor =
                c.preeditWithCursor(PinyinPreeditMode::RawText).second;
            FCITX_ASSERT(actualCursor == expectedCursor[i])
                << "Raw Cursor: " << i << " Cursor: " << actualCursor
                << " Expected: " << expectedCursor[i];
        }
        FCITX_ASSERT(c.preedit(PinyinPreeditMode::RawText) == "xi ' an");
    }

    return 0;
}
