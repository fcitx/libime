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
#include "lm/model.hh"
#include <type_traits>

namespace libime {

class StaticLanguageModelFilePrivate {
public:
    StaticLanguageModelFilePrivate(const char *file,
                                   const lm::ngram::Config &config)
        : model_(file, config) {}
    lm::ngram::QuantArrayTrieModel model_;
};

StaticLanguageModelFile::StaticLanguageModelFile(const char *file) {
    lm::ngram::Config config;
    config.sentence_marker_missing = lm::SILENT;
    d_ptr = std::make_unique<StaticLanguageModelFilePrivate>(file, config);
}

StaticLanguageModelFile::~StaticLanguageModelFile() {}

static_assert(sizeof(void *) + sizeof(lm::ngram::State) <= StateSize, "Size");

bool LanguageModelBase::isNodeUnknown(const LatticeNode &node) const {
    return isUnknown(node.idx(), node.word());
}

static_assert(std::is_pod<lm::ngram::State>::value, "State should be pod");
static_assert(std::is_same<WordIndex, lm::WordIndex>::value,
              "word index should be same type");

static inline lm::ngram::State &lmState(State &state) {
    return *reinterpret_cast<lm::ngram::State *>(state.data());
}
static inline const lm::ngram::State &lmState(const State &state) {
    return *reinterpret_cast<const lm::ngram::State *>(state.data());
}

class LanguageModelPrivate {
public:
    LanguageModelPrivate(std::shared_ptr<const StaticLanguageModelFile> file)
        : file_(file) {}

    auto &model() { return file_->d_func()->model_; }
    const auto &model() const { return file_->d_func()->model_; }

    std::shared_ptr<const StaticLanguageModelFile> file_;
    State beginState_;
    State nullState_;
    float unknown_ = std::log10(1 / 60000000.0f);
};

LanguageModel::LanguageModel(const char *file) : LanguageModel(std::make_shared<StaticLanguageModelFile>(file)) {
}

LanguageModel::LanguageModel(std::shared_ptr<const StaticLanguageModelFile> file) : LanguageModelBase(), d_ptr(std::make_unique<LanguageModelPrivate>(file))
{
    FCITX_D();
    lmState(d->beginState_) = d->model().BeginSentenceState();
    lmState(d->nullState_) = d->model().NullContextState();
}

LanguageModel::~LanguageModel() {}

WordIndex LanguageModel::beginSentence() const {
    FCITX_D();
    auto &v = d->model().GetVocabulary();
    return v.BeginSentence();
}

WordIndex LanguageModel::endSentence() const {
    FCITX_D();
    auto &v = d->model().GetVocabulary();
    return v.EndSentence();
}

WordIndex LanguageModel::unknown() const {
    FCITX_D();
    auto &v = d->model().GetVocabulary();
    return v.NotFound();
}

WordIndex LanguageModel::index(boost::string_view word) const {
    FCITX_D();
    auto &v = d->model().GetVocabulary();
    return v.Index(StringPiece{word.data(), word.size()});
}

const State &LanguageModel::beginState() const {
    FCITX_D();
    return d->beginState_;
}

const State &LanguageModel::nullState() const {
    FCITX_D();
    return d->nullState_;
}

float LanguageModel::score(const State &state, const WordNode &node,
                           State &out) const {
    FCITX_D();
    assert(&state != &out);
    return d->model().Score(lmState(state), node.idx(), lmState(out)) +
           (node.idx() == unknown() ? d->unknown_ : 0.0f);
}

bool LanguageModel::isUnknown(WordIndex idx, boost::string_view) const {
    return idx == unknown();
}

void LanguageModel::setUnknownPenalty(float unknown) {
    FCITX_D();
    d->unknown_ = unknown;
}

float LanguageModel::unknownPenalty() const {
    FCITX_D();
    return d->unknown_;
}
}
