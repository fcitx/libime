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
#include "decoder.h"
#include "languagemodel.h"
#include "pinyindictionary.h"
#include "pinyinencoder.h"
#include <iostream>

using namespace libime;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        return 1;
    }
    PinyinDictionary dict(argv[1], PinyinDictFormat::Text);
    LanguageModel model(argv[2]);
    Decoder decoder(&dict, &model);
    PinyinEncoder::parseUserPinyin("wojiushixiangceshi", PinyinFuzzyFlag::None)
        .dfs([&decoder](const PinyinSegments &pyseg,
                        const std::vector<size_t> &pos) {
            Segments segs(pyseg.pinyin(), pos);
            decoder.decode(segs, 1, {});
            return true;
        });
    PinyinEncoder::parseUserPinyin("xian", PinyinFuzzyFlag::Inner)
        .dfs([&decoder](const PinyinSegments &pyseg,
                        const std::vector<size_t> &pos) {
            Segments segs(pyseg.pinyin(), pos);
            decoder.decode(segs, 1, {});
            return true;
        });
    PinyinEncoder::parseUserPinyin("xiian", PinyinFuzzyFlag::Inner)
        .dfs([&decoder](const PinyinSegments &pyseg,
                        const std::vector<size_t> &pos) {
            Segments segs(pyseg.pinyin(), pos);
            decoder.decode(segs, 1, {});
            return true;
        });
    return 0;
}
