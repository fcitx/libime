/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/languagemodel.h"
#include "libime/core/lattice.h"
#include "testdir.h"
#include <cmath>
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
        std::cout << "Prob" << std::pow(10, s) << '\n';
        state = out_state;
        sum += s;
    }
    std::cout << sum << std::endl;

    return 0;
}
