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

int main() {
    using namespace libime;
    PinyinDictionary dict;
    dict.load(PinyinDictionary::SystemDict,
              LIBIME_BINARY_DIR "/data/dict.converted", PinyinDictFormat::Text);

    // add a manual dict
    std::stringstream ss;
    ss << "倪辉 ni'hui 0.0";
    dict.load(PinyinDictionary::UserDict, ss, PinyinDictFormat::Text);
    // dict.dump(std::cout);
    char c[] = {static_cast<char>(PinyinInitial::N), 0,
                static_cast<char>(PinyinInitial::H), 0};
    dict.matchWords(c, sizeof(c),
                    [c](std::string_view encodedPinyin, std::string_view hanzi,
                        float cost) {
                        std::cout
                            << PinyinEncoder::decodeFullPinyin(encodedPinyin)
                            << " " << hanzi << " " << cost << std::endl;
                        return true;
                    });

    dict.save(0, LIBIME_BINARY_DIR "/test/testpinyindictionary.dict",
              PinyinDictFormat::Binary);
    return 0;
}
