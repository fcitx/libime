/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "userlanguagemodel.h"
#include "constants.h"
#include "historybigram.h"
#include "lm/model.hh"
#include "utils.h"

namespace libime {

class UserLanguageModelPrivate {
public:
    State beginState_;
    State nullState_;

    HistoryBigram history_;
    float weight_ = DEFAULT_USER_LANGUAGE_MODEL_USER_WEIGHT;
    // log(wa * exp(a) + wb * exp(b))
    // log(exp(log(wa) + a) + exp(b + log(wb))
    float wa_ = std::log10(1 - weight_), wb_ = std::log10(weight_);

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
UserLanguageModel::UserLanguageModel(const char *file)
    : UserLanguageModel(std::make_shared<StaticLanguageModelFile>(file)) {}

UserLanguageModel::UserLanguageModel(
    std::shared_ptr<const StaticLanguageModelFile> file)
    : LanguageModel(file), d_ptr(std::make_unique<UserLanguageModelPrivate>()) {
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

const HistoryBigram &UserLanguageModel::history() const {
    FCITX_D();
    return d->history_;
}

void UserLanguageModel::load(std::istream &in) {
    FCITX_D();
    HistoryBigram history;
    history.setUnknownPenalty(d->history_.unknownPenalty());
    history.load(in);
    d->history_ = std::move(history);
}
void UserLanguageModel::save(std::ostream &out) {
    FCITX_D();
    return d->history_.save(out);
}

void UserLanguageModel::setHistoryWeight(float w) {
    FCITX_D();
    assert(w >= 0.0 && w <= 1.0);
    d->weight_ = w;
    d->wa_ = std::log10(1 - d->weight_);
    d->wb_ = std::log10(d->weight_);
}

const State &UserLanguageModel::beginState() const {
    FCITX_D();
    return d->beginState_;
}

const State &UserLanguageModel::nullState() const {
    FCITX_D();
    return d->nullState_;
}

static const float log_10 = std::log(10);

// log10(exp10(a) + exp10(b))
//   = log10(exp10(b) * (1 + exp10(a - b)))
//   = b + log10(1 + exp10(a - b))
//   = b + log1p(exp10(a - b)) / log(10)
inline float log1p10exp(float x) {
    return x < MIN_FLOAT_LOG10 ? 0. : std::log1p(std::pow(10, x)) / log_10;
}
inline float sum_log_prob(float a, float b) {
    return a > b ? (a + log1p10exp(b - a)) : (b + log1p10exp(a - b));
}

float UserLanguageModel::score(const State &state, const WordNode &word,
                               State &out) const {
    FCITX_D();
    float score = LanguageModel::score(state, word, out);
    auto prev = d->wordFromState(state);
    float userScore = d->history_.score(prev, &word);
    d->setWordToState(out, &word);
    return std::max(score, sum_log_prob(score + d->wa_, userScore + d->wb_));
}

bool UserLanguageModel::isUnknown(WordIndex idx, std::string_view view) const {
    FCITX_D();
    return idx == unknown() && d->history_.isUnknown(view);
}
} // namespace libime
