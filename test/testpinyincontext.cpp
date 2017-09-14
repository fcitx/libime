/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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

#include "libime/core/historybigram.h"
#include "libime/core/lattice.h"
#include "libime/core/userlanguagemodel.h"
#include "libime/pinyin/pinyincontext.h"
#include "libime/pinyin/pinyindecoder.h"
#include "libime/pinyin/pinyindictionary.h"
#include "libime/pinyin/pinyinime.h"
#include "testdir.h"
#include <boost/range/adaptor/transformed.hpp>
#include <fcitx-utils/log.h>
#include <fcitx-utils/stringutils.h>
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
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.select(40);
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.cancel();
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.type("shi'");
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    int i = 0;
    for (auto &candidate : c.candidates()) {
        if (candidate.toString() == "西安市") {
            break;
        }
        i++;
    }
    c.select(i);
    FCITX_ASSERT(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.select(1);
    FCITX_ASSERT(c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.clear();
    FCITX_ASSERT(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.type("zi'ji'ge'zi'");
    FCITX_ASSERT(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    i = 0;
    for (auto &candidate : c.candidates()) {
        if (candidate.toString() == "子集") {
            break;
        }
        i++;
    }
    c.select(i);
    FCITX_ASSERT(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }

    std::cout << "--------------------------------" << std::endl;
    i = 0;
    for (auto &candidate : c.candidates()) {
        if (candidate.toString() == "各自") {
            break;
        }
        i++;
    }
    c.select(i);
    FCITX_ASSERT(c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    c.clear();
    c.type("n");
    for (auto &candidate : c.candidates()) {
        for (auto node : candidate.sentence()) {
            auto &pinyin =
                static_cast<const PinyinLatticeNode *>(node)->encodedPinyin();
            std::cout << node->word();
            if (!pinyin.empty()) {
                std::cout << " " << PinyinEncoder::decodeFullPinyin(pinyin);
            }
        }
        std::cout << std::endl;
    }

    return 0;
}
