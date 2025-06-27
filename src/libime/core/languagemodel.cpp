/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "languagemodel.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <fcitx-utils/fs.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/stringutils.h>
#include "config.h"
#include "constants.h"
#include "datrie.h"
#include "lattice.h"
#include "lm/config.hh"
#include "lm/lm_exception.hh"
#include "lm/model.hh"
#include "lm/state.hh"
#include "lm/word_index.hh"
#include "util/string_piece.hh"

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
    State state = _state;
    State outState;
    std::vector<WordNode> nodes;
    for (auto word : words) {
        auto idx = index(word);
        nodes.emplace_back(word, idx);
        s += score(state, nodes.back(), outState);
        state = outState;
    }
    return s;
}

static_assert(std::is_standard_layout_v<lm::ngram::State> &&
                  std::is_trivial_v<lm::ngram::State>,
              "State should be pod");
static_assert(std::is_same_v<WordIndex, lm::WordIndex>,
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
        : file_(std::move(file)) {}

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
    : d_ptr(std::make_unique<LanguageModelPrivate>(std::move(file))) {
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
    const auto &v = d->model()->GetVocabulary();
    return v.BeginSentence();
}

WordIndex LanguageModel::endSentence() const {
    FCITX_D();
    if (!d->model()) {
        return 0;
    }
    const auto &v = d->model()->GetVocabulary();
    return v.EndSentence();
}

WordIndex LanguageModel::unknown() const {
    FCITX_D();
    if (!d->model()) {
        return 0;
    }
    const auto &v = d->model()->GetVocabulary();
    return v.NotFound();
}

WordIndex LanguageModel::index(std::string_view word) const {
    FCITX_D();
    if (!d->model()) {
        return 0;
    }
    const auto &v = d->model()->GetVocabulary();
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
           (node.idx() == unknown() ? d->unknown_ : 0.0F);
}

bool LanguageModel::isUnknown(WordIndex idx, std::string_view /*word*/) const {
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

    const char *modelDirs = getenv("LIBIME_MODEL_DIRS");
    std::vector<std::string> dirs;
    if (modelDirs && modelDirs[0]) {
        dirs = fcitx::stringutils::split(modelDirs, ":");
    } else {
        dirs.push_back(LIBIME_INSTALL_LIBDATADIR);
    }

    for (const auto &dir : dirs) {
        auto file = fcitx::stringutils::joinPath(dir, language + ".lm");
        if (fcitx::fs::isreg(file)) {
            return file;
        }
    }
    return {};
}
} // namespace libime
