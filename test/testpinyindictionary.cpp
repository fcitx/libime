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

#include "libime/pinyindictionary.h"
#include "libime/pinyinencoder.h"
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        return 1;
    }
    using namespace libime;
    PinyinDictionary dict(argv[1], PinyinDictFormat::Text);
    // dict.dump(std::cout);
    char c[] = {static_cast<char>(PinyinInitial::N), static_cast<char>(PinyinInitial::H),
                PinyinEncoder::initialFinalSepartor, 0, 0};
    dict.matchWords(c, sizeof(c),
                    [c](const char *encodedPinyin, const std::string &hanzi, float cost) {
                        std::cout << PinyinEncoder::decodeFullPinyin(encodedPinyin, sizeof(c))
                                  << " " << hanzi << " " << cost << std::endl;
                        return true;
                    });

    PinyinEncoder::parseUserPinyin("zuishengmengsi''", PinyinFuzzyFlag::None)
        .dfs([&dict](const PinyinSegments &pyseg, const std::vector<size_t> &pos) {
            Segments segs(pyseg.pinyin(), pos);
            for (size_t i = 0; i < segs.size(); i++) {
                std::cout << segs.at(i) << " ";
            }
            std::cout << std::endl;
            dict.matchPrefix(segs, 0, [&segs](size_t to, boost::string_view hanzi, float value) {
                std::cout << segs.prefix(to) << " " << hanzi << " " << value << std::endl;
            });
            return true;
        });
    dict.save(argv[2]);
    return 0;
}
