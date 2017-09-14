/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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

#include "libime/core/languagemodel.h"
#include "libime/core/lattice.h"
#include "testdir.h"
#include <fcitx-utils/log.h>

int main() {
    using namespace libime;
    LanguageModel model(LIBIME_BINARY_DIR "/data/sc.lm");
    State state(model.nullState()), out_state = model.nullState();
    std::string word;
    float sum = 0.0f;
    while (std::cin >> word) {
        float s;
        WordNode w(word, model.index(word));
        std::cout << w.idx() << " " << (s = model.score(state, w, out_state))
                  << '\n';
        state = out_state;
        sum += s;
    }
    std::cout << sum << std::endl;

    return 0;
}
