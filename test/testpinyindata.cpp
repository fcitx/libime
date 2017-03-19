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

#include "libime/pinyindata.h"
#include "libime/pinyinencoder.h"
#include <cassert>
#include <iostream>

using namespace libime;

int main() {
    std::set<std::string> seen;
    std::set<char> last;
    std::set<char> first;
    for (auto p : getPinyinMap()) {
        auto pinyin = p.pinyin();
        auto initial = p.initial();
        auto final = p.final();

        auto fullPinyin = PinyinEncoder::initialToString(initial) + PinyinEncoder::finalToString(final);
        assert(fullPinyin == PinyinEncoder::applyFuzzy(pinyin.to_string(), p.flags()));
        if (p.flags() == 0) {
            // make sure valid item is unique
            auto result = seen.insert(pinyin.to_string());
            assert(result.second);
        }
        last.insert(p.pinyin().back());
        first.insert(p.pinyin().front());
    }
    for (auto c : last) {
        std::cout << c;
    }
    for (auto c : first) {
        std::cout << c;
    }
    return 0;
}
