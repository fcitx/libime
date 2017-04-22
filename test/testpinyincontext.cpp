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

#include "languagemodel.h"
#include "lattice.h"
#include "pinyincontext.h"
#include "pinyindictionary.h"
#include "pinyinime.h"
#include <boost/range/adaptor/transformed.hpp>
#include <fcitx-utils/stringutils.h>
#include <iostream>

using namespace libime;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        return 1;
    }
    PinyinDictionary dict(argv[1], PinyinDictFormat::Binary);
    LanguageModel model(argv[2]);
    PinyinIME ime(&dict, &model);
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
    assert(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.select(1);
    assert(c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.clear();
    assert(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.type("zi'ji'ge'zi'");
    assert(!c.selected());
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
    assert(!c.selected());
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
    assert(c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }

    return 0;
}
