/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <cstddef>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string_view>
#include <fcitx-utils/log.h>
#include "libime/pinyin/pinyindictionary.h"
#include "libime/pinyin/pinyinencoder.h"
#include "testdir.h"

using namespace libime;

constexpr char testPinyin1[] = "ni'hui";
constexpr char testHanzi1[] = "倪辉";

constexpr char testPinyin2[] = "xiao'qi'e";
constexpr char testHanzi2[] = "小企鹅";

bool searchWord(const PinyinDictionary &dict, const char *data, size_t size,
                std::string_view expectedPinyin,
                std::string_view expectedHanzi) {
    bool seenWord = false;
    dict.matchWords(data, size,
                    [&seenWord, expectedHanzi,
                     expectedPinyin](std::string_view encodedPinyin,
                                     std::string_view hanzi, float cost) {
                        std::cout
                            << PinyinEncoder::decodeFullPinyin(encodedPinyin)
                            << " " << hanzi << " " << cost << std::endl;
                        if (hanzi == expectedHanzi &&
                            PinyinEncoder::decodeFullPinyin(encodedPinyin) ==
                                expectedPinyin) {
                            seenWord = true;
                        }
                        return true;
                    });
    return seenWord;
}

bool searchWordPrefix(const PinyinDictionary &dict, const char *data,
                      size_t size, std::string_view expectedPinyin,
                      std::string_view expectedHanzi) {
    bool seenWord = false;
    dict.matchWordsPrefix(
        data, size,
        [&seenWord, expectedHanzi,
         expectedPinyin](std::string_view encodedPinyin, std::string_view hanzi,
                         float cost) {
            std::cout << PinyinEncoder::decodeFullPinyin(encodedPinyin) << " "
                      << hanzi << " " << cost << std::endl;
            if (hanzi == expectedHanzi &&
                PinyinEncoder::decodeFullPinyin(encodedPinyin) ==
                    expectedPinyin) {
                seenWord = true;
            }
            return true;
        });
    return seenWord;
}

int main() {
    PinyinDictionary dict;
    dict.load(PinyinDictionary::SystemDict,
              LIBIME_BINARY_DIR "/data/dict_sc.txt", PinyinDictFormat::Text);

    // add a manual dict
    std::stringstream ss;
    ss << testHanzi1 << " " << testPinyin1 << " 0.0" << std::endl
       << testHanzi2 << " " << testPinyin2 << std::endl;
    dict.load(PinyinDictionary::UserDict, ss, PinyinDictFormat::Text);
    // dict.dump(std::cout);
    char c[] = {static_cast<char>(PinyinInitial::N), 0,
                static_cast<char>(PinyinInitial::H), 0};
    char c2[] = {static_cast<char>(PinyinInitial::X),    0,
                 static_cast<char>(PinyinInitial::Q),    0,
                 static_cast<char>(PinyinInitial::Zero), 0};

    FCITX_ASSERT(searchWord(dict, c, sizeof(c), testPinyin1, testHanzi1));
    FCITX_ASSERT(searchWord(dict, c2, sizeof(c2), testPinyin2, testHanzi2));
    // Search x q as prefix and see we we can find xiao qi e.
    FCITX_ASSERT(searchWordPrefix(dict, c2, 4, testPinyin2, testHanzi2));

    // Remove the word and check again.
    dict.removeWord(PinyinDictionary::UserDict, testPinyin1, testHanzi1);
    FCITX_ASSERT(!searchWord(dict, c, sizeof(c), testPinyin1, testHanzi1));
    FCITX_ASSERT(!searchWordPrefix(dict, c, 2, testPinyin1, testHanzi1));

    dict.save(0, LIBIME_BINARY_DIR "/test/testpinyindictionary.dict",
              PinyinDictFormat::Binary);
    return 0;
}
