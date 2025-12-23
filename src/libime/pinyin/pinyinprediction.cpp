/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "pinyinprediction.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/misc.h>
#include <fcitx-utils/stringutils.h>
#include "libime/core/languagemodel.h"
#include "libime/core/prediction.h"
#include "libime/pinyin/pinyindictionary.h"

namespace libime {

class PinyinPredictionPrivate {
public:
    const PinyinDictionary *dict_ = nullptr;
};

PinyinPrediction::PinyinPrediction()
    : d_ptr(std::make_unique<PinyinPredictionPrivate>()) {}

PinyinPrediction::~PinyinPrediction() {}

void PinyinPrediction::setPinyinDictionary(const PinyinDictionary *dict) {
    FCITX_D();
    d->dict_ = dict;
}

std::vector<std::pair<std::string, PinyinPredictionSource>>
PinyinPrediction::predict(const State &state,
                          const std::vector<std::string> &sentence,
                          std::string_view lastEncodedPinyin, size_t maxSize) {
    FCITX_D();
    std::vector<std::pair<std::string, PinyinPredictionSource>> finalResult;

    if (lastEncodedPinyin.empty() || sentence.empty()) {
        auto result = Prediction::predictWithScore(state, sentence, maxSize);
        std::transform(result.begin(), result.end(),
                       std::back_inserter(finalResult),
                       [](std::pair<std::string, float> &value) {
                           return std::make_pair(std::move(value.first),
                                                 PinyinPredictionSource::Model);
                       });
        return finalResult;
    }

    auto cmp = [](auto &lhs, auto &rhs) {
        if (std::get<float>(lhs) != std::get<float>((rhs))) {
            return std::get<float>(lhs) > std::get<float>(rhs);
        }
        return std::get<std::string>(lhs) < std::get<std::string>(rhs);
    };

    auto result = Prediction::predictWithScore(state, sentence, maxSize);
    std::vector<std::tuple<std::string, float, PinyinPredictionSource>>
        intermedidateResult;
    std::transform(
        result.begin(), result.end(), std::back_inserter(intermedidateResult),
        [](std::pair<std::string, float> &value) {
            return std::make_tuple(std::move(value.first), value.second,
                                   PinyinPredictionSource::Model);
        });
    std::make_heap(intermedidateResult.begin(), intermedidateResult.end(), cmp);

    State prevState = model()->nullState();
    State outState;
    std::vector<WordNode> nodes;
    std::unordered_set<std::string> dup;
    if (!sentence.empty()) {
        nodes.reserve(sentence.size());
        for (const auto &word : fcitx::MakeIterRange(
                 sentence.begin(), std::prev(sentence.end()))) {
            auto idx = model()->index(word);
            nodes.emplace_back(word, idx);
            model()->score(prevState, nodes.back(), outState);
            prevState = outState;
        }
        // We record the last score for the sentence word to adjust the partial
        // score. E.g. for 无, model may contain 压力 and dict contain 聊 score
        // of 聊 should be P(...|无聊) and score of 压力 should be P(...|无) *
        // P(...无|压力) adjust is the P(...|无) here.
        nodes.emplace_back(sentence.back(), model()->index(sentence.back()));
        float adjust = model()->score(prevState, nodes.back(), outState);
        for (auto &result : intermedidateResult) {
            std::get<float>(result) += adjust;
            dup.insert(std::get<std::string>(result));
        }
    }

    d->dict_->matchWordsPrefix(
        lastEncodedPinyin.data(), lastEncodedPinyin.size(),
        [this, &sentence, &prevState, &cmp, &intermedidateResult, &dup,
         maxSize](std::string_view, std::string_view hz, float cost) {
            if (sentence.back().size() < hz.size() &&
                hz.starts_with(sentence.back())) {

                std::string newWord(hz.substr(sentence.back().size()));
                if (dup.contains(newWord)) {
                    return true;
                }

                std::tuple<std::string, float, PinyinPredictionSource> newItem{
                    std::move(newWord),
                    cost + model()->singleWordScore(prevState, hz),
                    PinyinPredictionSource::Dictionary};

                dup.insert(std::get<std::string>(newItem));
                intermedidateResult.push_back(std::move(newItem));
                std::push_heap(intermedidateResult.begin(),
                               intermedidateResult.end(), cmp);
                while (intermedidateResult.size() > maxSize) {
                    std::pop_heap(intermedidateResult.begin(),
                                  intermedidateResult.end(), cmp);
                    dup.erase(
                        std::get<std::string>(intermedidateResult.back()));
                    intermedidateResult.pop_back();
                }
            }
            return true;
        });

    std::sort_heap(intermedidateResult.begin(), intermedidateResult.end(), cmp);
    std::transform(intermedidateResult.begin(), intermedidateResult.end(),
                   std::back_inserter(finalResult), [](auto &value) {
                       return std::make_pair(
                           std::move(std::get<std::string>(value)),
                           std::get<PinyinPredictionSource>(value));
                   });

    return finalResult;
}

std::vector<std::string>
PinyinPrediction::predict(const std::vector<std::string> &sentence,
                          size_t maxSize) {
    return Prediction::predict(sentence, maxSize);
}

} // namespace libime
