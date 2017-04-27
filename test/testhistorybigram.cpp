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

#include "libime/historybigram.h"
#include <iostream>
#include <sstream>

int main() {
    using namespace libime;
    HistoryBigram history;
    history.setUnknown(std::log10(1.0f / 150000));
    history.add({"你", "是", "一个", "好人"});
    history.add({"我", "是", "一个", "坏人"});
    history.add({"他"});
    std::cout << history.score("你", "是") << std::endl;
    std::cout << history.score("他", "是") << std::endl;
    std::cout << history.score("他", "不是") << std::endl;
    {
        std::stringstream ss;
        history.save(ss);
        history.clear();
        std::cout << history.score("你", "是") << std::endl;
        std::cout << history.score("他", "是") << std::endl;
        std::cout << history.score("他", "不是") << std::endl;
        history.load(ss);
    }
    std::cout << history.score("你", "是") << std::endl;
    std::cout << history.score("他", "是") << std::endl;
    std::cout << history.score("他", "不是") << std::endl;
    {
        std::stringstream ss;
        try {
            history.load(ss);
        } catch (const std::exception &e) {
            history.clear();
        }
    }
    std::cout << history.score("你", "是") << std::endl;
    std::cout << history.score("他", "是") << std::endl;
    std::cout << history.score("他", "不是") << std::endl;

    return 0;
}
