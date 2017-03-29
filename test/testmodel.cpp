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

#include "libime/languagemodel.h"
#include <iostream>

int main(int argc, char *argv[]) {
    using namespace libime;
    LanguageModel model(argv[1]);
    State state(model.nullState()), out_state = model.nullState();
    std::string word;
    float sum = 0.0f;
    while (std::cin >> word) {
        float s;
        std::cout << (s = model.score(state, model.index(word), out_state))
                  << '\n';
        state = out_state;
        sum += s;
    }
    std::cout << sum << std::endl;

    return 0;
}
