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

static_assert(std::is_same<WordIndex, lm::WordIndex>::value, "word index should be same type");

class LanguageModelPrivate {
public:
    LanguageModelPrivate(const char *file) : model_(file) {}

    lm::ngram::TrieModel model_;
};

LanguageModel::LanguageModel(const char *file)
    : d_ptr(std::make_unique<LanguageModelPrivate>(file)) {}

LanguageModel::~LanguageModel() {}

WordIndex LanguageModel::beginSentence() {
    FCITX_D();
    auto &v = d->model_.GetVocabulary();
    return v.BeginSentence();
}

WordIndex LanguageModel::endSentence() {
    FCITX_D();
    auto &v = d->model_.GetVocabulary();
    return v.EndSentence();
}

WordIndex LanguageModel::index(boost::string_view word) {
    FCITX_D();
    auto &v = d->model_.GetVocabulary();
    return v.Index(StringPiece{word.data(), static_cast<int32_t>(word.size())});
}
}
