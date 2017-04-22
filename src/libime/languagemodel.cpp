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
#include "lm/model.hh"
#include <type_traits>

namespace libime {

static_assert(std::is_pod<lm::ngram::State>::value, "State should be pod");
static_assert(std::is_same<WordIndex, lm::WordIndex>::value,
              "word index should be same type");

lm::ngram::State &lmState(State &state) {
    return *reinterpret_cast<lm::ngram::State *>(state.data());
}
const lm::ngram::State &lmState(const State &state) {
    return *reinterpret_cast<const lm::ngram::State *>(state.data());
}

class LanguageModelPrivate {
public:
    LanguageModelPrivate(const char *file, lm::ngram::Config config)
        : model_(file, config) {}

    lm::ngram::QuantArrayTrieModel model_;
};

LanguageModel::LanguageModel(const char *file) {
    lm::ngram::Config config;
    config.sentence_marker_missing = lm::SILENT;
    d_ptr = std::make_unique<LanguageModelPrivate>(file, config);
}

LanguageModel::~LanguageModel() {}

WordIndex LanguageModel::beginSentence() const {
    FCITX_D();
    auto &v = d->model_.GetVocabulary();
    return v.BeginSentence();
}

WordIndex LanguageModel::endSentence() const {
    FCITX_D();
    auto &v = d->model_.GetVocabulary();
    return v.EndSentence();
}

WordIndex LanguageModel::unknown() const {
    FCITX_D();
    auto &v = d->model_.GetVocabulary();
    return v.NotFound();
}

WordIndex LanguageModel::index(boost::string_view word) const {
    FCITX_D();
    auto &v = d->model_.GetVocabulary();
    return v.Index(StringPiece{word.data(), static_cast<int32_t>(word.size())});
}

State LanguageModel::beginState() const {
    FCITX_D();
    State state;
    state.resize(sizeof(lm::ngram::State));
    lmState(state) = d->model_.NullContextState();
    return state;
}

State LanguageModel::nullState() const {
    FCITX_D();
    State state;
    state.resize(sizeof(lm::ngram::State));
    lmState(state) = d->model_.NullContextState();
    return state;
}

float LanguageModel::score(const State &state, WordIndex word,
                           State &out) const {
    FCITX_D();
    assert(&state != &out);
    return d->model_.Score(lmState(state), word, lmState(out));
}
}
