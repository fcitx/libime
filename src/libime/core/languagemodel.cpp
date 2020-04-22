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

#include "languagemodel.h"
#include "config.h"
#include "constants.h"
#include "lattice.h"
#include "lm/model.hh"
#include <fcitx-utils/fs.h>
#include <fstream>
#include <type_traits>

namespace libime {

class StaticLanguageModelFilePrivate {
public:
    StaticLanguageModelFilePrivate(const char *file,
                                   const lm::ngram::Config &config)
        : model_(file, config), file_(file) {}
    lm::ngram::QuantArrayTrieModel model_;
    std::string file_;
    mutable bool predictionLoaded_ = false;
    mutable DATrie<float> prediction_;
};

StaticLanguageModelFile::StaticLanguageModelFile(const char *file) {
    lm::ngram::Config config;
    config.sentence_marker_missing = lm::SILENT;
    d_ptr = std::make_unique<StaticLanguageModelFilePrivate>(file, config);
}

StaticLanguageModelFile::~StaticLanguageModelFile() {}

const DATrie<float> &StaticLanguageModelFile::predictionTrie() const {
    FCITX_D();
    if (!d->predictionLoaded_) {
        d->predictionLoaded_ = true;
        try {
            std::ifstream fin;
            fin.open(d->file_ + ".predict", std::ios::in | std::ios::binary);
            if (fin) {
                DATrie<float> trie;
                trie.load(fin);
                d->prediction_ = std::move(trie);
            }
        } catch (...) {
        }
    }
    return d->prediction_;
}

static_assert(sizeof(void *) + sizeof(lm::ngram::State) <= StateSize, "Size");

bool LanguageModelBase::isNodeUnknown(const LatticeNode &node) const {
    return isUnknown(node.idx(), node.word());
}

float LanguageModelBase::singleWordScore(std::string_view word) const {
    auto idx = index(word);
    State dummy;
    WordNode node(word, idx);
    return score(nullState(), node, dummy);
}

float LanguageModelBase::singleWordScore(const State &state,
                                         std::string_view word) const {
    return wordsScore(state, std::vector<std::string_view>{word});
}

float LanguageModelBase::wordsScore(
    const State &_state, const std::vector<std::string_view> &words) const {
    float s = 0;
    State state = _state, outState;
    std::vector<WordNode> nodes;
    for (auto word : words) {
        auto idx = index(word);
        nodes.emplace_back(word, idx);
        s += score(state, nodes.back(), outState);
        state = std::move(outState);
    }
    return s;
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

    auto *model() { return file_ ? &file_->d_func()->model_ : nullptr; }
    const auto *model() const {
        return file_ ? &file_->d_func()->model_ : nullptr;
    }

    std::shared_ptr<const StaticLanguageModelFile> file_;
    State beginState_;
    State nullState_;
    float unknown_ =
        std::log10(DEFAULT_LANGUAGE_MODEL_UNKNOWN_PROBABILITY_PENALTY);
};

LanguageModel::LanguageModel(const char *file)
    : LanguageModel(std::make_shared<StaticLanguageModelFile>(file)) {}

LanguageModel::LanguageModel(
    std::shared_ptr<const StaticLanguageModelFile> file)
    : LanguageModelBase(), d_ptr(std::make_unique<LanguageModelPrivate>(file)) {
    FCITX_D();
    if (d->model()) {
        lmState(d->beginState_) = d->model()->BeginSentenceState();
        lmState(d->nullState_) = d->model()->NullContextState();
    }
}

LanguageModel::~LanguageModel() {}

std::shared_ptr<const StaticLanguageModelFile>
LanguageModel::languageModelFile() const {
    FCITX_D();
    return d->file_;
}

WordIndex LanguageModel::beginSentence() const {
    FCITX_D();
    if (!d->model()) {
        return 0;
    }
    auto &v = d->model()->GetVocabulary();
    return v.BeginSentence();
}

WordIndex LanguageModel::endSentence() const {
    FCITX_D();
    if (!d->model()) {
        return 0;
    }
    auto &v = d->model()->GetVocabulary();
    return v.EndSentence();
}

WordIndex LanguageModel::unknown() const {
    FCITX_D();
    if (!d->model()) {
        return 0;
    }
    auto &v = d->model()->GetVocabulary();
    return v.NotFound();
}

WordIndex LanguageModel::index(std::string_view word) const {
    FCITX_D();
    if (!d->model()) {
        return 0;
    }
    auto &v = d->model()->GetVocabulary();
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
    if (!d->model()) {
        return d->unknown_;
    }
    return d->model()->Score(lmState(state), node.idx(), lmState(out)) +
           (node.idx() == unknown() ? d->unknown_ : 0.0f);
}

bool LanguageModel::isUnknown(WordIndex idx, std::string_view) const {
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

class LanguageModelResolverPrivate {
public:
    std::unordered_map<std::string,
                       std::weak_ptr<const StaticLanguageModelFile>>
        files_;
};

LanguageModelResolver::LanguageModelResolver()
    : d_ptr(std::make_unique<LanguageModelResolverPrivate>()) {}

FCITX_DEFINE_DEFAULT_DTOR_AND_MOVE(LanguageModelResolver)

std::shared_ptr<const StaticLanguageModelFile>
LanguageModelResolver::languageModelFileForLanguage(
    const std::string &language) {
    FCITX_D();
    auto iter = d->files_.find(language);
    std::shared_ptr<const StaticLanguageModelFile> file;
    if (iter != d->files_.end()) {
        file = iter->second.lock();
        if (file) {
            return file;
        }
        d->files_.erase(iter);
    }

    auto fileName = languageModelFileNameForLanguage(language);
    if (fileName.empty()) {
        return nullptr;
    }

    file = std::make_shared<StaticLanguageModelFile>(fileName.data());
    d->files_.emplace(language, file);
    return file;
}

DefaultLanguageModelResolver::DefaultLanguageModelResolver() = default;
DefaultLanguageModelResolver::~DefaultLanguageModelResolver() = default;

DefaultLanguageModelResolver &DefaultLanguageModelResolver::instance() {
    static DefaultLanguageModelResolver resolver;
    return resolver;
}

std::string DefaultLanguageModelResolver::languageModelFileNameForLanguage(
    const std::string &language) {
    if (language.empty() || language.find('/') != std::string::npos) {
        return {};
    }
    auto file = fcitx::stringutils::joinPath(LIBIME_INSTALL_PKGDATADIR,
                                             language + ".lm");
    if (fcitx::fs::isreg(file)) {
        return file;
    }
    return {};
}
} // namespace libime
