/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <algorithm>
#include <iterator>
#include <memory>
#include <fcitx-utils/log.h>
#include "libime/core/userlanguagemodel.h"
#include "libime/pinyin/pinyincontext.h"
#include "libime/pinyin/pinyincorrectionprofile.h"
#include "libime/pinyin/pinyindictionary.h"
#include "libime/pinyin/pinyinencoder.h"
#include "libime/pinyin/pinyinime.h"
#include "libime/pinyin/shuangpinprofile.h"
#include "testdir.h"

using namespace libime;

int main() {
    fcitx::Log::setLogRule("libime=5");
    libime::PinyinIME ime(
        std::make_unique<PinyinDictionary>(),
        std::make_unique<UserLanguageModel>(LIBIME_BINARY_DIR "/data/sc.lm"));
    ime.setNBest(2);
    ime.dict()->load(PinyinDictionary::SystemDict,
                     LIBIME_BINARY_DIR "/data/sc.dict",
                     PinyinDictFormat::Binary);
    PinyinFuzzyFlags flags = PinyinFuzzyFlag::Inner;
    ime.setFuzzyFlags(flags);
    ime.setScoreFilter(1.0F);
    ime.setShuangpinProfile(
        std::make_shared<ShuangpinProfile>(ShuangpinBuiltinProfile::Xiaohe));
    PinyinContext c(&ime);

    c.type("nihaozhongguo");
    FCITX_ASSERT(c.candidates().size() == c.candidateSet().size());
    FCITX_ASSERT(c.candidateSet().count("你好中国"));
    c.setCursor(5);
    FCITX_ASSERT(c.candidates().size() == c.candidateSet().size());
    FCITX_ASSERT(c.candidatesToCursor().size() ==
                 c.candidatesToCursorSet().size());
    FCITX_ASSERT(c.candidates().size() != c.candidatesToCursor().size())
        << c.candidatesToCursorSet();
    FCITX_ASSERT(!c.candidatesToCursorSet().count("你好中国"));
    FCITX_ASSERT(c.candidatesToCursorSet().count("你好"));
    c.setCursor(0);
    auto iter = std::find_if(
        c.candidates().begin(), c.candidates().end(),
        [](const auto &cand) { return cand.toString() == "你好中国"; });
    FCITX_ASSERT(iter != c.candidates().end());
    FCITX_ASSERT(!ime.dict()->lookupWord(PinyinDictionary::UserDict,
                                         "ni'hao'zhong'guo", "你好中国"));
    c.select(std::distance(c.candidates().begin(), iter));
    c.learn();
    FCITX_ASSERT(ime.dict()->lookupWord(PinyinDictionary::UserDict,
                                        "ni'hao'zhong'guo", "你好中国"));

    c.setUseShuangpin(true);

    c.type("bkqilb");
    FCITX_ASSERT(c.candidates().size() == c.candidateSet().size());
    FCITX_ASSERT(c.candidateSet().count("冰淇淋"));
    c.clear();

    c.type("bkqiln");
    FCITX_ASSERT(c.candidates().size() == c.candidateSet().size());
    FCITX_ASSERT(!c.candidateSet().count("冰淇淋"));
    c.clear();

    ime.setCorrectionProfile(std::make_shared<PinyinCorrectionProfile>(
        BuiltinPinyinCorrectionProfile::Qwerty));
    ime.setShuangpinProfile(std::make_shared<libime::ShuangpinProfile>(
        ShuangpinBuiltinProfile::Xiaohe, ime.correctionProfile().get()));
    ime.setFuzzyFlags(flags | PinyinFuzzyFlag::Correction);

    c.type("bkqiln");
    FCITX_ASSERT(c.candidates().size() == c.candidateSet().size());
    FCITX_ASSERT(c.candidateSet().count("冰淇淋"));
    c.clear();

    return 0;
}
