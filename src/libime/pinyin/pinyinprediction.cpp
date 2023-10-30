#include "pinyinprediction.h"
#include "libime/core/prediction.h"
#include "libime/pinyin/pinyindictionary.h"
#include <algorithm>
#include <fcitx-utils/misc.h>
#include <fcitx-utils/stringutils.h>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

    State prevState = model()->nullState(), outState;
    std::vector<WordNode> nodes;
    if (sentence.size() >= 1) {
        nodes.reserve(sentence.size() - 1);
        for (const auto &word : fcitx::MakeIterRange(
                 sentence.begin(), std::prev(sentence.end()))) {
            auto idx = model()->index(word);
            nodes.emplace_back(word, idx);
            model()->score(prevState, nodes.back(), outState);
            prevState = std::move(outState);
        }
    }

    d->dict_->matchWordsPrefix(
        lastEncodedPinyin.data(), lastEncodedPinyin.size(),
        [this, &sentence, &prevState, &cmp, &intermedidateResult,
         maxSize](std::string_view, std::string_view hz, float cost) {
            if (sentence.back().size() < hz.size() &&
                fcitx::stringutils::startsWith(hz, sentence.back())) {

                std::tuple<std::string, float, PinyinPredictionSource> newItem{
                    std::string(hz.substr(sentence.back().size())),
                    cost + model()->singleWordScore(prevState, hz),
                    PinyinPredictionSource::Dictionary};
                intermedidateResult.push_back(std::move(newItem));
                std::push_heap(intermedidateResult.begin(),
                               intermedidateResult.end(), cmp);
                while (intermedidateResult.size() > maxSize) {
                    std::pop_heap(intermedidateResult.begin(),
                                  intermedidateResult.end(), cmp);
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