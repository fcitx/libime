/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/userlanguagemodel.h"
#include "libime/pinyin/pinyincontext.h"
#include "libime/pinyin/pinyindictionary.h"
#include "libime/pinyin/pinyinencoder.h"
#include "libime/pinyin/pinyinime.h"
#include "libime/pinyin/shuangpinprofile.h"
#include "testdir.h"
#include <fcitx-utils/log.h>
#include <memory>

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
    ime.setFuzzyFlags(PinyinFuzzyFlag::Inner);
    ime.setScoreFilter(1.0f);
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
    return 0;
}
