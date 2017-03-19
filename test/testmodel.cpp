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

#include "lm/model.hh"

int main(int argc, char *argv[]) {
    using namespace lm::ngram;
    lm::ngram::Config config;
    config.sentence_marker_missing = lm::SILENT;
    TrieModel model(argv[1], config);
    State state(model.BeginSentenceState()), out_state;
    const auto &vocab = model.GetVocabulary();
    std::string word;
    while (std::cin >> word) {
        std::cout << model.Score(state, vocab.Index(word), out_state) << '\n';
        state = out_state;
    }

    return 0;
}
