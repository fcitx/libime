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
#include "historybigram.h"
#include "lm/model.hh"
#include "utils.h"

namespace libime {

class UserLanguageModelPrivate {
public:
    State beginState_;
    State nullState_;

    HistoryBigram history_;
    float weight_ = 0.7;

    const WordNode *wordFromState(const State &state) const {
        return load_data<const WordNode *>(reinterpret_cast<const char *>(
            state.data() + sizeof(lm::ngram::State)));
    }

    void setWordToState(State &state, const WordNode *node) const {
        return store_data<const WordNode *>(
            reinterpret_cast<char *>(state.data() + sizeof(lm::ngram::State)),
            node);
    }
};

UserLanguageModel::UserLanguageModel(const char *sysFile)
    : LanguageModel(sysFile),
      d_ptr(std::make_unique<UserLanguageModelPrivate>()) {
    FCITX_D();
    // resize will fill remaining with zero
    d->beginState_ = LanguageModel::beginState();
    d->setWordToState(d->beginState_, nullptr);
    d->nullState_ = LanguageModel::nullState();
    d->setWordToState(d->nullState_, nullptr);
}

UserLanguageModel::~UserLanguageModel() {}

HistoryBigram &UserLanguageModel::history() {
    FCITX_D();
    return d->history_;
}

void UserLanguageModel::load(std::istream &in) {
    FCITX_D();
    HistoryBigram history;
    history.setUnknownPenalty(d->history_.unknownPenalty());
    history.load(in);
    using std::swap;
    swap(d->history_, history);
}
void UserLanguageModel::save(std::ostream &out) {
    FCITX_D();
    return d->history_.save(out);
}

void UserLanguageModel::setHistoryWeight(float w) {
    FCITX_D();
    assert(w >= 0.0 && w <= 1.0);
    d->weight_ = w;
}

const State &UserLanguageModel::beginState() const {
    FCITX_D();
    return d->beginState_;
}

const State &UserLanguageModel::nullState() const {
    FCITX_D();
    return d->nullState_;
}

float UserLanguageModel::score(const State &state, const WordNode &word,
                               State &out) const {
    FCITX_D();
    float score = LanguageModel::score(state, word, out);
    auto prev = d->wordFromState(state);
    float userScore = d->history_.score(prev, &word);
    d->setWordToState(out, &word);
    return score * (1 - d->weight_) + userScore * d->weight_;
}

bool UserLanguageModel::isUnknown(WordIndex idx,
                                  boost::string_view view) const {
    FCITX_D();
    return idx == unknown() && d->history_.isUnknown(view);
}
}
