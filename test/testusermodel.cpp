/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/historybigram.h"
#include "libime/core/lattice.h"
#include "libime/core/userlanguagemodel.h"
#include "testdir.h"
#include <cmath>
#include <fcitx-utils/log.h>
#include <fstream>
#include <list>

int main(int argc, char *argv[]) {
    using namespace libime;
    UserLanguageModel model(LIBIME_BINARY_DIR "/data/sc.lm");
    if (argc >= 2) {
        std::fstream fin(argv[1], std::ios::in | std::ios::binary);
        model.history().load(fin);
    }
    State state(model.nullState()), out_state = model.nullState();
    std::string word;
    float sum = 0.0f;
    std::list<WordNode> nodes;
    while (std::cin >> word) {
        float s;
        nodes.emplace_back(word, model.index(word));
        std::cout << nodes.back().idx() << " "
                  << (s = model.score(state, nodes.back(), out_state)) << '\n';
        std::cout << "Prob" << std::pow(10, s) << '\n';
        state = out_state;
        sum += s;
    }
    std::cout << sum << std::endl;

    return 0;
}
