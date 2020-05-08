/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "prediction.h"
#include "datrie.h"
#include "historybigram.h"
#include <unordered_set>

namespace libime {

class PredictionPrivate {
public:
    const LanguageModel *model_ = nullptr;
    const HistoryBigram *bigram_ = nullptr;
};

Prediction::Prediction() : d_ptr(std::make_unique<PredictionPrivate>()) {}

Prediction::~Prediction() = default;

void Prediction::setLanguageModel(const LanguageModel *model) {
    FCITX_D();
    d->model_ = model;
}

void Prediction::setHistoryBigram(const HistoryBigram *bigram) {
    FCITX_D();
    d->bigram_ = bigram;
}
std::vector<std::string>
Prediction::predict(const std::vector<std::string> &sentence,
                    size_t realMaxSize) {
    FCITX_D();
    if (!d->model_) {
        return {};
    }

    std::vector<WordNode> node;
    State state = d->model_->nullState(), outState;
    std::vector<WordNode> nodes;
    for (auto word : sentence) {
        auto idx = d->model_->index(word);
        nodes.emplace_back(word, idx);
        d->model_->score(state, nodes.back(), outState);
        state = std::move(outState);
    }
    return predict(state, sentence, realMaxSize);
}

std::vector<std::string>
Prediction::predict(const State &state,
                    const std::vector<std::string> &sentence,
                    size_t realMaxSize) {
    FCITX_D();
    if (!d->model_) {
        return {};
    }
    // Search more get less.
    size_t maxSize = realMaxSize * 2;
    std::unordered_set<std::string> words;

    if (auto file = d->model_->languageModelFile()) {
        std::string search = "<unk>";
        if (!sentence.empty()) {
            search = sentence.back();
        }
        search += "|";
        auto &trie = file->predictionTrie();
        trie.foreach(search, [&trie, &words,
                              maxSize](DATrie<float>::value_type, size_t len,
                                       DATrie<float>::position_type pos) {
            std::string buf;
            trie.suffix(buf, len, pos);
            words.emplace(std::move(buf));

            if (maxSize > 0 && words.size() >= maxSize) {
                return false;
            }
            return true;
        });
    }

    if (d->bigram_) {
        d->bigram_->fillPredict(words, sentence, maxSize);
    }

    std::vector<std::string> result;
    if (!d->model_) {
        result.insert(result.end(), words.begin(), words.end());
        std::sort(result.begin(), result.end());
    } else {
        std::vector<std::pair<float, std::string>> temps;
        for (auto word : words) {
            auto score = d->model_->singleWordScore(state, word);
            temps.emplace_back(score, std::move(word));
        }
        std::sort(temps.begin(), temps.end(), [](auto &lhs, auto &rhs) {
            if (lhs.first != rhs.first) {
                return lhs.first > rhs.first;
            } else {
                return lhs.second < rhs.second;
            }
        });
        for (auto &temp : temps) {
            result.emplace_back(std::move(temp.second));
        }
    }

    if (result.size() > realMaxSize) {
        result.resize(realMaxSize);
    }

    return result;
}

} // namespace libime
