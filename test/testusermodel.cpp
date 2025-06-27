/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <cmath>
#include <fstream>
#include <ios>
#include <iostream>
#include <list>
#include <ostream>
#include <string>
#include "libime/core/historybigram.h"
#include "libime/core/languagemodel.h"
#include "libime/core/lattice.h"
#include "libime/core/userlanguagemodel.h"
#include "testdir.h"

int main(int argc, char *argv[]) {
    using namespace libime;
    UserLanguageModel model(LIBIME_BINARY_DIR "/data/sc.lm");
    if (argc >= 2) {
        std::fstream fin(argv[1], std::ios::in | std::ios::binary);
        model.history().load(fin);
    }
    State state(model.nullState());
    State out_state = model.nullState();
    std::string word;
    float sum = 0.0F;
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
