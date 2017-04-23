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

#include "userlanguagemodel.h"
#include "utils.h"

namespace libime {

class UserLanguageModelPrivate {
public:
    size_t stateSize = 0;
    State beginState_;
    State nullState_;

    WordNode *wordFromState(const State &state) {
        return load_data<WordNode *>(
            reinterpret_cast<const char *>(state.data() + stateSize));
    }

    void setWordToState(State &state, WordNode *node) {
        return store_data<WordNode *>(
            reinterpret_cast<char *>(state.data() + stateSize), node);
    }
};

UserLanguageModel::UserLanguageModel(const char *sysFile, const char *userFile)
    : LanguageModel(sysFile),
      d_ptr(std::make_unique<UserLanguageModelPrivate>()) {
    FCITX_D();
    // resize will fill remaining with zero
    d->beginState_ = LanguageModel::beginState();
    d->stateSize = d->beginState_.size();
    d->beginState_.resize(d->beginState_.size() + sizeof(void *));
    d->nullState_ = LanguageModel::nullState();
    d->nullState_.resize(d->nullState_.size() + sizeof(void *));
}

UserLanguageModel::~UserLanguageModel() {}

const State &UserLanguageModel::beginState() const {
    FCITX_D();
    return d->beginState_;
}

const State &UserLanguageModel::nullState() const {
    FCITX_D();
    return d->nullState_;
}

float UserLanguageModel::score(const State &state, const WordNode *word,
                               State &out) const {
    float score = LanguageModel::score(state, word, out);
    return score / 2;
}
}
