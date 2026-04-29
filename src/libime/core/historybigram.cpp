/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "historybigram.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <istream>
#include <iterator>
#include <list>
#include <memory>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/stringutils.h>
#include "constants.h"
#include "datrie.h"
#include "lattice.h"
#include "utils_p.h"
#include "zstdfilter.h"

namespace libime {

namespace {

using WordWithCode = HistoryBigram::WordWithCode;
using WordWithCodeView = HistoryBigram::WordWithCodeView;

constexpr uint32_t historyBinaryFormatMagic = 0x000fc315;
constexpr uint32_t historyBinaryFormatVersion = 0x4;
constexpr char bigramSeparator = '\x01';
constexpr char wordCodeSeparator = '\x02';

std::string wordAndCodeToString(WordWithCodeView wordAndCode) {
    std::string s{std::get<0>(wordAndCode)};
    if (s.empty()) {
        return s;
    }
    auto code = std::get<1>(wordAndCode);
    if (!code.empty()) {
        s += wordCodeSeparator;
        s += code;
    }
    return s;
}

WordWithCode bigramWordWithCode(WordWithCodeView prev, WordWithCodeView cur) {
    std::string s;
    s.append(std::get<0>(prev));
    s += bigramSeparator;
    s.append(std::get<0>(cur));

    auto code1 = std::get<1>(prev);
    auto code2 = std::get<1>(cur);
    std::string concatCode;
    if (code1.empty() && code2.empty()) {
        concatCode = "";
    } else {
        concatCode = code1;
        concatCode += bigramSeparator;
        concatCode += code2;
    }
    return {s, concatCode};
}

struct WeightedTrie {
    using TrieType = DATrie<int32_t>;

public:
    WeightedTrie() = default;

    void clear() { trie_.clear(); }

    const TrieType &trie() const { return trie_; }

    int32_t weightedSize() const { return weightedSize_; }

    int32_t freq(WordWithCodeView wordAndCode) const {
        // If query with code, the match will be {word, ""} + {word, code}.
        // If query without code, the match will be {word, ""} + {word,
        // separator}.
        TrieType::position_type pos = 0;
        auto result = 0;
        auto v = trie_.traverse(wordAndCode.first, pos);
        if (TrieType::isValid(v)) {
            result += v;
        } else if (TrieType::isNoPath(v)) {
            return 0;
        }
        const char separator[] = {wordCodeSeparator, '\0'};
        v = trie_.traverse(separator, pos);
        if (!TrieType::isNoPath(v)) {
            if (!wordAndCode.second.empty() &&
                wordAndCode.second.front() != bigramSeparator &&
                wordAndCode.second.back() != bigramSeparator) {
                v = trie_.traverse(wordAndCode.second, pos);
                if (TrieType::isValid(v)) {
                    result += v;
                }
            } else {
                trie_.foreach(
                    [this, &result, &wordAndCode](TrieType::value_type value,
                                                  size_t len,
                                                  TrieType::position_type pos) {
                        if (len == 0) {
                            return true;
                        }
                        if (!wordAndCode.second.empty()) {
                            assert(
                                wordAndCode.second.front() == bigramSeparator ||
                                wordAndCode.second.back() == bigramSeparator);
                            std::string codeInTrie;
                            trie().suffix(codeInTrie, len, pos);
                            if (wordAndCode.second.front() == bigramSeparator &&
                                !codeInTrie.ends_with(wordAndCode.second)) {
                                return true;
                            }
                            if (wordAndCode.second.back() == bigramSeparator &&
                                !codeInTrie.starts_with(wordAndCode.second)) {
                                return true;
                            }
                        }
                        result += value;
                        return true;
                    },
                    pos);
            }
        }
        return result;
    }

    void incFreq(WordWithCodeView wordAndCode, int32_t delta) {
        auto s = wordAndCodeToString(wordAndCode);

        trie_.update(s.data(), s.size(),
                     [delta](int32_t v) { return v + delta; });
        weightedSize_ += delta;
    }

    void decFreq(WordWithCodeView wordAndCode, int32_t delta) {
        auto s = wordAndCodeToString(wordAndCode);
        auto v = trie_.exactMatchSearch(s.data(), s.size());
        if (TrieType::isNoValue(v)) {
            return;
        }
        if (v <= delta) {
            trie_.erase(s.data(), s.size());
            decWeightedSize(v);
        } else {
            v -= delta;
            trie_.set(s.data(), s.size(), v);
            decWeightedSize(delta);
        }
    }

    void fillPredict(std::unordered_set<std::string> &words,
                     std::string_view word, size_t maxSize) const {
        trie_.foreach(word,
                      [this, &words, maxSize](TrieType::value_type, size_t len,
                                              TrieType::position_type pos) {
                          std::string buf;
                          trie().suffix(buf, len, pos);
                          auto separatorPos = buf.find(wordCodeSeparator);
                          if (separatorPos != std::string::npos) {
                              buf.erase(separatorPos);
                          }
                          // Skip special word.
                          if (buf == "<s>" || buf == "</s>") {
                              return true;
                          }
                          words.emplace(std::move(buf));

                          return maxSize <= 0 || words.size() < maxSize;
                      });
    }

private:
    void decWeightedSize(int32_t v) {
        weightedSize_ -= v;
        weightedSize_ = std::max(weightedSize_, 0);
    }

    int32_t weightedSize_ = 0;
    TrieType trie_;
};

class HistoryBigramPool {
public:
    HistoryBigramPool(size_t maxSize) : maxSize_(maxSize) {}

    void load(std::istream &in) {
        clear();
        uint32_t count = 0;
        throw_if_io_fail(unmarshall(in, count));
        while (count--) {
            uint32_t size = 0;
            throw_if_io_fail(unmarshall(in, size));
            std::vector<WordWithCode> sentence;
            while (size--) {
                std::string buffer;
                throw_if_io_fail(unmarshallString(in, buffer));
                std::string_view bufferView{buffer};
                size_t separatorPos = bufferView.find(wordCodeSeparator);
                if (separatorPos != std::string_view::npos) {
                    sentence.emplace_back(
                        std::string(bufferView.substr(0, separatorPos)),
                        std::string(bufferView.substr(separatorPos + 1)));
                } else {
                    sentence.emplace_back(std::move(buffer), "");
                }
            }
            add(sentence);
        }
    }

    void loadText(std::istream &in) {
        clear();
        std::string buf;
        std::vector<std::string> lines;
        while (std::getline(in, buf)) {
            lines.emplace_back(buf);
            if (lines.size() >= maxSize_) {
                break;
            }
        }
        for (auto &line : lines | std::views::reverse) {
            std::string_view lineView{line};
            std::vector<std::string> tokens;
            bool withCode = false;
            while (!lineView.empty()) {
                std::string token;
                auto consumed = fcitx::stringutils::consumeMaybeEscapedValue(
                    lineView, FCITX_WHITESPACE, &token);
                if (!consumed.empty()) {
                    tokens.push_back(std::move(token));
                }
                if (tokens.size() == 1 && !lineView.empty() &&
                    lineView.front() == '\t') {
                    withCode = true;
                }
            }

            if (withCode) {
                if (tokens.size() % 2 != 0) {
                    continue;
                }
                add(std::views::iota(static_cast<size_t>(0),
                                     tokens.size() / 2) |
                    std::views::transform([&tokens](size_t i) {
                        return WordWithCode{tokens[i * 2], tokens[(i * 2) + 1]};
                    }));

            } else {
                add(tokens |
                    std::views::transform([](const auto &word) -> WordWithCode {
                        std::vector<std::string> wordWithMaybeCode =
                            fcitx::stringutils::split(
                                word, "\t",
                                fcitx::stringutils::SplitBehavior::KeepEmpty);
                        if (wordWithMaybeCode.size() == 2) {
                            return WordWithCode{wordWithMaybeCode[0],
                                                wordWithMaybeCode[1]};
                        }
                        return WordWithCode{word, ""};
                    }));
            }
        }
    }

    void save(std::ostream &out) {
        uint32_t count = recent_.size();
        throw_if_io_fail(marshall(out, count));
        // When we do save, we need to reverse the history order.
        // Because loading the history is done by call "add", which basically
        // expect the history from old to new.
        for (auto &sentence : recent_ | std::views::reverse) {
            uint32_t size = sentence.size();
            throw_if_io_fail(marshall(out, size));
            for (const auto &s : sentence) {
                throw_if_io_fail(marshallString(out, wordAndCodeToString(s)));
            }
        }
    }

    void dump(std::ostream &out) const {
        for (const auto &sentence : recent_) {
            bool first = true;
            bool hasCode = std::ranges::any_of(sentence, [](const auto &item) {
                return !std::get<1>(item).empty();
            });
            for (const auto &s : sentence) {
                if (first) {
                    first = false;
                } else {
                    out << " ";
                }
                out << fcitx::stringutils::escapeForValue(std::get<0>(s));
                if (hasCode) {
                    out << "\t"
                        << fcitx::stringutils::escapeForValue(std::get<1>(s));
                }
            }
            out << '\n';
        }
    }

    void clear() {
        recent_.clear();
        unigram_.clear();
        bigram_.clear();
        size_ = 0;
    }

    template <typename R>
    std::list<std::vector<WordWithCode>> add(const R &sentence) {
        std::list<std::vector<WordWithCode>> popedSentence;
        if (sentence.empty()) {
            return popedSentence;
        }
        // Validate data.
        if (std::ranges::any_of(sentence, [](const auto &item) {
                const auto &[word, code] = item;
                return word.find('\0') != std::string::npos;
            })) {
            return popedSentence;
        }
        while (recent_.size() >= maxSize_) {
            remove(recent_.back());
            popedSentence.splice(popedSentence.end(), recent_,
                                 std::prev(recent_.end()));
        }

        std::vector<WordWithCode> newSentence;
        auto delta = 1;
        for (auto iter = sentence.begin(), end = sentence.end(); iter != end;
             iter++) {
            unigram_.incFreq(*iter, delta);
            auto next = std::ranges::next(iter);
            if (next != end) {
                incBigram(*iter, *next, delta);
            }
            newSentence.push_back(*iter);
        }
        recent_.push_front(std::move(newSentence));
        unigram_.incFreq({"<s>", ""}, delta);
        unigram_.incFreq({"</s>", ""}, delta);
        incBigram({"<s>", ""}, sentence.front(), delta);
        incBigram(sentence.back(), {"</s>", ""}, delta);

        return popedSentence;
    }

    int32_t unigramFreq(WordWithCodeView s) const { return unigram_.freq(s); }

    int32_t bigramFreq(WordWithCodeView s1, WordWithCodeView s2) const {
        return bigram_.freq(bigramWordWithCode(s1, s2));
    }

    bool isUnknown(WordWithCodeView word) const {
        return unigramFreq(word) == 0;
    }

    size_t maxSize() const { return maxSize_; }

    size_t realSize() const { return recent_.size(); }

    void forget(std::string_view word, std::string_view code) {
        auto iter = recent_.begin();
        while (iter != recent_.end()) {
            if (std::find_if(
                    iter->begin(), iter->end(), [word, code](const auto &item) {
                        const auto &[w, c] = item;
                        return w == word && (code.empty() || c == code);
                    }) != iter->end()) {
                remove(*iter);
                iter = recent_.erase(iter);
            } else {
                ++iter;
            }
        }
    }

    void fillPredict(std::unordered_set<std::string> &words,
                     std::string_view word, size_t maxSize = 0) const {
        bigram_.fillPredict(words, word, maxSize);
    }

    bool maybeAppendToLatestSentence(const std::vector<WordWithCode> &context,
                                     std::vector<WordWithCode> &newSentence) {
        if (recent_.empty() || newSentence.empty()) {
            return false;
        }
        auto &latestSentence = recent_.front();
        if (latestSentence.size() < context.size() ||
            !std::ranges::equal(
                context,
                std::views::drop(latestSentence,
                                 latestSentence.size() - context.size()))) {
            return false;
        }

        const int delta = 1;
        decBigram(latestSentence.back(), {"</s>", ""}, delta);
        for (auto &item : newSentence) {
            unigram_.incFreq(item, delta);
            incBigram(latestSentence.back(), item, delta);
            latestSentence.push_back(std::move(item));
        }
        incBigram(latestSentence.back(), {"</s>", ""}, delta);

        return true;
    }

private:
    template <typename R>
    void remove(const R &sentence) {
        const int delta = 1;
        for (auto iter = sentence.begin(), end = sentence.end(); iter != end;
             iter++) {
            unigram_.decFreq(*iter, delta);
            auto next = std::next(iter);
            if (next != end) {
                decBigram(*iter, *next, delta);
            }
        }
        decBigram({"<s>", ""}, sentence.front(), delta);
        decBigram(sentence.back(), {"</s>", ""}, delta);
    }

    void decBigram(WordWithCodeView s1, WordWithCodeView s2, int32_t delta) {
        bigram_.decFreq(bigramWordWithCode(s1, s2), delta);
    }

    void incBigram(WordWithCodeView s1, WordWithCodeView s2, int delta) {
        bigram_.incFreq(bigramWordWithCode(s1, s2), delta);
    }

    const size_t maxSize_;

    // Used when maxSize_ != 0.
    size_t size_ = 0;
    std::list<std::vector<WordWithCode>> recent_;

    // Used for look up
    WeightedTrie unigram_;
    WeightedTrie bigram_;
};

} // namespace

// We define the frequency as following.
// (1 - p) the frequency belongs to first pool.
// p * (1 - p) Second pool
// p^2 * (1 - p) Third pool
// ...
// p^(n-1) n-th pool.
// In sum, it's (1-p) * p^(i - 1)
// And then we define alpha as p = 1 / (1 + alpha).
class HistoryBigramPrivate {
public:
    void populateSentence(std::list<std::vector<WordWithCode>> popedSentence) {
        for (size_t i = 1; !popedSentence.empty() && i < pools_.size(); i++) {
            std::list<std::vector<WordWithCode>> nextSentences;
            while (!popedSentence.empty()) {
                auto newPopedSentence = pools_[i].add(popedSentence.front());
                popedSentence.pop_front();
                nextSentences.splice(nextSentences.end(), newPopedSentence);
            }
            popedSentence = std::move(nextSentences);
        }
    }

    float unigramFreq(WordWithCodeView word) const {
        assert(pools_.size() == poolWeight_.size());
        float freq = 0;
        for (size_t i = 0; i < pools_.size(); i++) {
            freq += pools_[i].unigramFreq(word) * poolWeight_[i];
        }
        return freq;
    }

    float bigramFreq(WordWithCodeView prev, WordWithCodeView cur) const {
        assert(pools_.size() == poolWeight_.size());
        float freq = 0;
        for (size_t i = 0; i < pools_.size(); i++) {
            freq += pools_[i].bigramFreq(prev, cur) * poolWeight_[i];
        }
        return freq;
    }

    float unigramSize() const {
        float size = 0;
        for (size_t i = 0; i < pools_.size(); i++) {
            size += pools_[i].maxSize() * poolWeight_[i];
        }
        return size;
    }

    // A log probabilty.
    float unknown_ =
        std::log10(DEFAULT_LANGUAGE_MODEL_UNKNOWN_PROBABILITY_PENALTY);
    bool useOnlyUnigram_ = false;
    std::vector<HistoryBigramPool> pools_;
    std::vector<float> poolWeight_;
};

HistoryBigram::HistoryBigram()
    : d_ptr(std::make_unique<HistoryBigramPrivate>()) {
    FCITX_D();
    const float p = 1.0 / (1 + HISTORY_BIGRAM_ALPHA_VALUE);
    constexpr std::array<int, 3> poolSize = {128, 8192, 65536};
    d->pools_.reserve(poolSize.size());
    d->poolWeight_.reserve(poolSize.size());
    for (auto size : poolSize) {
        d->pools_.emplace_back(size);
        float portion = 1.0F;
        if (d->pools_.size() != poolSize.size()) {
            portion *= 1 - p;
        }
        portion *= std::pow(p, d->pools_.size() - 1);
        d->poolWeight_.push_back(portion / d->pools_.back().maxSize());
    }
    setUnknownPenalty(
        std::log10(DEFAULT_LANGUAGE_MODEL_UNKNOWN_PROBABILITY_PENALTY));
}

FCITX_DEFINE_DEFAULT_DTOR_AND_MOVE(HistoryBigram)

void HistoryBigram::setUnknownPenalty(float unknown) {
    FCITX_D();
    d->unknown_ = unknown;
}

float HistoryBigram::unknownPenalty() const {
    FCITX_D();
    return d->unknown_;
}

void HistoryBigram::setUseOnlyUnigram(bool useOnlyUnigram) {
    FCITX_D();
    d->useOnlyUnigram_ = useOnlyUnigram;
}

bool HistoryBigram::useOnlyUnigram() const {
    FCITX_D();
    return d->useOnlyUnigram_;
}

void HistoryBigram::add(const libime::SentenceResult &sentence) {
    addWithCode(sentence, nullptr);
}

void HistoryBigram::addWithCode(
    const libime::SentenceResult &sentence,
    const ValidationCodeExtractor &validationCodeExtractor) {
    FCITX_D();
    d->populateSentence(d->pools_[0].add(
        sentence.sentence() |
        std::views::transform(
            [&validationCodeExtractor](const auto &item) -> WordWithCode {
                return {item->word(), validationCodeExtractor
                                          ? validationCodeExtractor(item)
                                          : ""};
            })));
}

void HistoryBigram::add(const std::vector<std::string> &sentence) {
    FCITX_D();
    d->populateSentence(d->pools_[0].add(
        sentence | std::views::transform([](const auto &word) -> WordWithCode {
            return WordWithCode{word, ""};
        })));
}

void HistoryBigram::addWithCode(
    const std::vector<WordWithCode> &sentenceWithValidationCode) {
    FCITX_D();
    d->populateSentence(d->pools_[0].add(sentenceWithValidationCode));
}

bool HistoryBigram::isUnknown(std::string_view v) const {
    FCITX_D();
    return std::ranges::all_of(d->pools_, [v](const HistoryBigramPool &pool) {
        return pool.isUnknown({v, ""});
    });
}

float HistoryBigram::score(std::string_view prev, std::string_view cur) const {
    return scoreWithCode({prev, ""}, {cur, ""});
}

float HistoryBigram::scoreWithCode(WordWithCodeView prev,
                                   WordWithCodeView cur) const {
    FCITX_D();
    if (prev.first.empty()) {
        prev.first = "<s>";
    }
    if (cur.first.empty()) {
        cur.first = "<unk>";
    }

    auto uf0 = d->unigramFreq(prev);
    auto bf = d->bigramFreq(prev, cur);
    auto uf1 = d->unigramFreq(cur);

    float bigramWeight = d->useOnlyUnigram_ ? 0.0F : 0.8F;
    // add 0.5 to avoid div 0
    float pr = 0.0F;
    pr += bigramWeight * bf / float(uf0 + (d->poolWeight_[0] / 2));
    pr += (1.0F - bigramWeight) * uf1 /
          float(d->unigramSize() + (d->poolWeight_[0] / 2));

    pr = std::min<double>(pr, 1.0);
    if (pr == 0) {
        return d->unknown_;
    }

    return std::log10(pr);
}

void HistoryBigram::load(std::istream &in) {
    FCITX_D();
    uint32_t magic = 0;
    uint32_t version = 0;
    throw_if_io_fail(unmarshall(in, magic));
    if (magic != historyBinaryFormatMagic) {
        throw std::invalid_argument("Invalid history magic.");
    }
    throw_if_io_fail(unmarshall(in, version));
    switch (version) {
    case 1:
        std::ranges::for_each(d->pools_ | std::views::take(2),
                              [&in](auto &pool) { pool.load(in); });
        break;
    case 2:
        std::ranges::for_each(d->pools_, [&in](auto &pool) { pool.load(in); });
        break;
    case 3:
    case historyBinaryFormatVersion:
        // For version 3 and version 4, the format is the same, but version 4
        // contains additional code data, bump the version to it not backward
        // compatible with version 3.
        readZSTDCompressed(in, [d](std::istream &compressIn) {
            std::ranges::for_each(d->pools_, [&compressIn](auto &pool) {
                pool.load(compressIn);
            });
        });
        break;
    default:
        throw std::invalid_argument("Invalid history version.");
    }
}

void HistoryBigram::loadText(std::istream &in) {
    FCITX_D();
    std::ranges::for_each(d->pools_, [&in](auto &pool) { pool.loadText(in); });
}

void HistoryBigram::save(std::ostream &out) {
    FCITX_D();
    throw_if_io_fail(marshall(out, historyBinaryFormatMagic));
    throw_if_io_fail(marshall(out, historyBinaryFormatVersion));

    writeZSTDCompressed(out, [d](std::ostream &compressOut) {
        std::ranges::for_each(
            d->pools_, [&compressOut](auto &pool) { pool.save(compressOut); });
    });
}

void HistoryBigram::dump(std::ostream &out) {
    FCITX_D();
    std::ranges::for_each(d->pools_,
                          [&out](const auto &pool) { pool.dump(out); });
}

void HistoryBigram::clear() {
    FCITX_D();
    std::ranges::for_each(d->pools_, std::mem_fn(&HistoryBigramPool::clear));
}

void HistoryBigram::forget(std::string_view word) { forget(word, ""); }

void HistoryBigram::forget(std::string_view word, std::string_view code) {
    FCITX_D();
    std::ranges::for_each(
        d->pools_, [word, code](auto &pool) { pool.forget(word, code); });
}

void HistoryBigram::fillPredict(std::unordered_set<std::string> &words,
                                const std::vector<std::string> &sentence,
                                size_t maxSize) const {
    FCITX_D();
    if (maxSize > 0 && words.size() >= maxSize) {
        return;
    }
    std::string lookup;
    if (!sentence.empty()) {
        lookup = sentence.back();
    } else {
        lookup = "<s>";
    }
    lookup += bigramSeparator;
    std::ranges::for_each(
        d->pools_, [&words, &lookup, maxSize](const HistoryBigramPool &pool) {
            pool.fillPredict(words, lookup, maxSize);
        });
}

bool HistoryBigram::containsBigram(std::string_view prev,
                                   std::string_view cur) const {
    FCITX_D();
    return std::ranges::any_of(
        d->pools_, [&prev, &cur](const HistoryBigramPool &pool) {
            return pool.bigramFreq({prev, ""}, {cur, ""}) > 0;
        });
}

float HistoryBigram::unigramFrequency(WordWithCodeView word) const {
    FCITX_D();
    return d->unigramFreq(word);
}

float HistoryBigram::bigramFrequency(WordWithCodeView prev,
                                     WordWithCodeView cur) const {
    FCITX_D();
    return d->bigramFreq(prev, cur);
}

int32_t HistoryBigram::rawUnigramFrequency(WordWithCodeView word) const {
    FCITX_D();
    int32_t freq = 0;
    for (const auto &pool : d->pools_) {
        freq += pool.unigramFreq(word);
    }
    return freq;
}

int32_t HistoryBigram::rawBigramFrequency(WordWithCodeView prev,
                                          WordWithCodeView cur) const {
    FCITX_D();
    int32_t freq = 0;
    for (const auto &pool : d->pools_) {
        freq += pool.bigramFreq(prev, cur);
    }
    return freq;
}

float HistoryBigram::score(const WordNode *prev, const WordNode *cur) const {
    return scoreWithCode(prev, cur, nullptr);
}

float HistoryBigram::scoreWithCode(
    const WordNode *prev, const WordNode *cur,
    const ValidationCodeExtractor &extractor) const {
    return scoreWithCode(
        {prev ? prev->word() : "", extractor && prev ? extractor(prev) : ""},
        {cur ? cur->word() : "", extractor && cur ? extractor(cur) : ""});
}

void HistoryBigram::addWithContext(const std::vector<WordWithCode> &context,
                                   std::vector<WordWithCode> newSentence) {
    FCITX_D();
    if (context.empty() ||
        !d->pools_[0].maybeAppendToLatestSentence(context, newSentence)) {
        addWithCode(newSentence);
    }
}

} // namespace libime
