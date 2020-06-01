/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/pinyin/pinyindictionary.h"
#include "libime/pinyin/pinyinencoder.h"
#include "testdir.h"
#include "testutils.h"
#include <fcitx-utils/log.h>
#include <sstream>

constexpr char testPinyin[] = "ni'hui";
constexpr char testHanzi[] = "倪辉";

int main() {
    using namespace libime;
    PinyinDictionary dict;
    dict.load(PinyinDictionary::SystemDict,
              LIBIME_BINARY_DIR "/data/dict.converted", PinyinDictFormat::Text);

    // add a manual dict
    std::stringstream ss;
    ss << testHanzi << " " << testPinyin << " 0.0";
    dict.load(PinyinDictionary::UserDict, ss, PinyinDictFormat::Text);
    // dict.dump(std::cout);
    char c[] = {static_cast<char>(PinyinInitial::N), 0,
                static_cast<char>(PinyinInitial::H), 0};

    bool seenWord = false;
    dict.matchWords(
        c, sizeof(c),
        [&seenWord](std::string_view encodedPinyin, std::string_view hanzi,
                    float cost) {
            std::cout << PinyinEncoder::decodeFullPinyin(encodedPinyin) << " "
                      << hanzi << " " << cost << std::endl;
            if (hanzi == testHanzi &&
                PinyinEncoder::decodeFullPinyin(encodedPinyin) == testPinyin) {
                seenWord = true;
            }
            return true;
        });
    FCITX_ASSERT(seenWord);

    // Remove the word and check again.
    dict.removeWord(PinyinDictionary::UserDict, testPinyin, testHanzi);
    seenWord = false;
    dict.matchWords(
        c, sizeof(c),
        [&seenWord](std::string_view encodedPinyin, std::string_view hanzi,
                    float cost) {
            std::cout << PinyinEncoder::decodeFullPinyin(encodedPinyin) << " "
                      << hanzi << " " << cost << std::endl;
            if (hanzi == testHanzi &&
                PinyinEncoder::decodeFullPinyin(encodedPinyin) == testPinyin) {
                seenWord = true;
            }
            return true;
        });
    FCITX_ASSERT(!seenWord);

    dict.save(0, LIBIME_BINARY_DIR "/test/testpinyindictionary.dict",
              PinyinDictFormat::Binary);
    return 0;
}
