/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "historybigram.h"
#include "constants.h"
#include "datrie.h"
#include "utils.h"
#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm.hpp>
#include <cmath>
#include <fcitx-utils/log.h>

namespace libime {

static constexpr uint32_t historyBinaryFormatMagic = 0x000fc315;
static constexpr uint32_t historyBinaryFormatVersion = 0x2;

struct WeightedTrie {
    using TrieType = DATrie<int32_t>;

public:
    WeightedTrie() = default;

    void clear() { trie_.clear(); }

    const TrieType &trie() const { return trie_; }

    int32_t weightedSize() const { return weightedSize_; }

    int32_t freq(std::string_view s) const {
        auto v = trie_.exactMatchSearch(s.data(), s.size());
        if (trie_.isNoValue(v)) {
            return 0;
        }
        return v;
    }

    void incFreq(std::string_view s, int32_t delta) {
        trie_.update(s.data(), s.size(),
                     [delta](int32_t v) { return v + delta; });
        weightedSize_ += delta;
    }

    void decFreq(std::string_view s, int32_t delta) {
        auto v = trie_.exactMatchSearch(s.data(), s.size());
        if (trie_.isNoValue(v)) {
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

    void eraseByKey(std::string_view s) {
        auto v = trie_.exactMatchSearch(s.data(), s.size());
        if (trie_.isNoValue(v)) {
            return;
        }
        trie_.erase(s);
        decWeightedSize(v);
    }

    void eraseByPrefix(std::string_view s) {
        std::vector<std::pair<std::string, int32_t>> values;
        trie_.foreach(s, [this, &values](TrieType::value_type value, size_t len,
                                         TrieType::position_type pos) {
            std::string buf;
            trie().suffix(buf, len, pos);
            values.emplace_back(std::move(buf), value);
            return true;
        });
        for (auto &value : values) {
            trie_.erase(value.first);
            decWeightedSize(value.second);
        }
    }

    void eraseBySuffix(std::string_view s) {
        std::vector<std::pair<std::string, int32_t>> values;
        trie_.foreach(s,
                      [this, &values, s](TrieType::value_type value, size_t len,
                                         TrieType::position_type pos) {
                          std::string buf;
                          trie().suffix(buf, len, pos);
                          if (boost::ends_with(buf, s)) {
                              values.emplace_back(std::move(buf), value);
                          }
                          return true;
                      });
        for (auto &value : values) {
            trie_.erase(value.first);
            decWeightedSize(value.second);
        }
    }

    void fillPredict(std::unordered_set<std::string> &words,
                     std::string_view word, size_t maxSize) const {
        trie_.foreach(word,
                      [this, &words, maxSize](TrieType::value_type, size_t len,
                                              TrieType::position_type pos) {
                          std::string buf;
                          trie().suffix(buf, len, pos);
                          // Skip special word.
                          if (buf == "<s>" || buf == "</s>") {
                              return true;
                          }
                          words.emplace(std::move(buf));

                          if (maxSize > 0 && words.size() >= maxSize) {
                              return false;
                          }
                          return true;
                      });
    }

private:
    void decWeightedSize(int32_t v) {
        weightedSize_ -= v;
        if (weightedSize_ < 0) {
            // This should not happen.
            weightedSize_ = 0;
        }
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
            std::vector<std::string> sentence;
            while (size--) {
                std::string buffer;
                throw_if_io_fail(unmarshallString(in, buffer));
                sentence.emplace_back(std::move(buffer));
            }
            add(sentence);
        }
    }

    void save(std::ostream &out) {
        uint32_t count = recent_.size();
        throw_if_io_fail(marshall(out, count));
        // When we do save, we need to reverse the history order.
        // Because loading the history is done by call "add", which basically
        // expect the history from old to new.
        for (auto &sentence : recent_ | boost::adaptors::reversed) {
            uint32_t size = sentence.size();
            throw_if_io_fail(marshall(out, size));
            for (auto &s : sentence) {
                throw_if_io_fail(marshallString(out, s));
            }
        }
    }

    void dump(std::ostream &out) const {
        for (const auto &sentence : recent_) {
            bool first = true;
            for (const auto &s : sentence) {
                if (first) {
                    first = false;
                } else {
                    out << " ";
                }
                out << s;
            }
            out << std::endl;
        }
    }

    void clear() {
        recent_.clear();
        unigram_.clear();
        bigram_.clear();
        size_ = 0;
    }

    template <typename R>
    std::list<std::vector<std::string>> add(const R &sentence) {
        std::list<std::vector<std::string>> popedSentence;
        if (sentence.empty()) {
            return popedSentence;
        }
        while (recent_.size() >= maxSize_) {
            remove(recent_.back());
            popedSentence.splice(popedSentence.end(), recent_,
                                 std::prev(recent_.end()));
        }

        std::vector<std::string> newSentence;
        auto delta = 1;
        for (auto iter = sentence.begin(), end = sentence.end(); iter != end;
             iter++) {
            unigram_.incFreq(*iter, delta);
            auto next = std::next(iter);
            if (next != end) {
                incBigram(*iter, *next, delta);
            }
            std::string ss;
            ss += *iter;
            newSentence.push_back(ss);
        }
        recent_.push_front(std::move(newSentence));
        unigram_.incFreq("<s>", delta);
        unigram_.incFreq("</s>", delta);
        incBigram("<s>", sentence.front(), delta);
        incBigram(sentence.back(), "</s>", delta);

        return popedSentence;
    }

    float unigramFreq(std::string_view s) const {
        auto v = unigram_.freq(s);
        return v;
    }

    float bigramFreq(std::string_view s1, std::string_view s2) const {
        std::string s;
        s.append(s1.data(), s1.size());
        s += '|';
        s.append(s2.data(), s2.size());
        auto v = bigram_.freq(s);
        return v;
    }

    bool isUnknown(std::string_view word) const {
        return unigramFreq(word) == 0;
    }

    size_t maxSize() const { return maxSize_; }

    size_t realSize() const { return recent_.size(); }

    void forget(std::string_view word) {
        auto iter = recent_.begin();
        while (iter != recent_.end()) {
            if (std::find(iter->begin(), iter->end(), word) != iter->end()) {
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
        decBigram("<s>", sentence.front(), delta);
        decBigram(sentence.back(), "</s>", delta);
    }

    void decBigram(std::string_view s1, std::string_view s2, int32_t delta) {
        std::string ss;
        ss.append(s1.data(), s1.size());
        ss += '|';
        ss.append(s2.data(), s2.size());
        bigram_.decFreq(ss, delta);
    }

    void incBigram(std::string_view s1, std::string_view s2, int delta) {
        std::string ss;
        ss.append(s1.data(), s1.size());
        ss += '|';
        ss.append(s2.data(), s2.size());
        bigram_.incFreq(ss, delta);
    }

    const size_t maxSize_;

    // Used when maxSize_ != 0.
    size_t size_ = 0;
    std::list<std::vector<std::string>> recent_;

    // Used for look up
    WeightedTrie unigram_;
    WeightedTrie bigram_;
};

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
    HistoryBigramPrivate() {}

    void populateSentence(std::list<std::vector<std::string>> popedSentence) {
        for (size_t i = 1; !popedSentence.empty() && i < pools_.size(); i++) {
            std::list<std::vector<std::string>> nextSentences;
            while (!popedSentence.empty()) {
                auto newPopedSentence = pools_[i].add(popedSentence.front());
                popedSentence.pop_front();
                nextSentences.splice(nextSentences.end(), newPopedSentence);
            }
            popedSentence = std::move(nextSentences);
        }
    }

    float unigramFreq(std::string_view word) const {
        assert(pools_.size() == poolWeight_.size());
        float freq = 0;
        for (size_t i = 0; i < pools_.size(); i++) {
            freq += pools_[i].unigramFreq(word) * poolWeight_[i];
        }
        return freq;
    }

    float bigramFreq(std::string_view prev, std::string_view cur) const {
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
    for (auto size : poolSize) {
        d->pools_.emplace_back(size);
        float portion = 1.0f;
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
    FCITX_D();
    d->populateSentence(
        d->pools_[0].add(sentence.sentence() |
                         boost::adaptors::transformed(
                             [](const auto &item) { return item->word(); })));
}

void HistoryBigram::add(const std::vector<std::string> &sentence) {
    FCITX_D();
    d->populateSentence(d->pools_[0].add(sentence));
}

bool HistoryBigram::isUnknown(std::string_view v) const {
    FCITX_D();
    return boost::algorithm::all_of(
        d->pools_,
        [v](const HistoryBigramPool &pool) { return pool.isUnknown(v); });
}

float HistoryBigram::score(std::string_view prev, std::string_view cur) const {
    FCITX_D();
    if (prev.empty()) {
        prev = "<s>";
    }
    if (cur.empty()) {
        cur = "<unk>";
    }

    auto uf0 = d->unigramFreq(prev);
    auto bf = d->bigramFreq(prev, cur);
    auto uf1 = d->unigramFreq(cur);

    float bigramWeight = d->useOnlyUnigram_ ? 0.0f : 0.68f;
    // add 0.5 to avoid div 0
    float pr = 0.0f;
    pr += bigramWeight * float(bf) / float(uf0 + d->poolWeight_[0] / 2);
    pr += (1.0f - bigramWeight) * float(uf1) /
          float(d->unigramSize() + d->poolWeight_[0] / 2);

    if (pr >= 1.0) {
        pr = 1.0;
    }
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
        std::for_each(d->pools_.begin(), d->pools_.begin() + 2,
                      [&in](auto &pool) { pool.load(in); });
        break;
    case historyBinaryFormatVersion:
        boost::range::for_each(d->pools_, [&in](auto &pool) { pool.load(in); });
        break;
    default: {
        throw std::invalid_argument("Invalid history version.");
    }
    }
}

void HistoryBigram::save(std::ostream &out) {
    FCITX_D();
    throw_if_io_fail(marshall(out, historyBinaryFormatMagic));
    throw_if_io_fail(marshall(out, historyBinaryFormatVersion));
    boost::range::for_each(d->pools_, [&out](auto &pool) { pool.save(out); });
}

void HistoryBigram::dump(std::ostream &out) {
    FCITX_D();
    boost::range::for_each(d->pools_,
                           [&out](const auto &pool) { pool.dump(out); });
}

void HistoryBigram::clear() {
    FCITX_D();
    boost::range::for_each(d->pools_, std::mem_fn(&HistoryBigramPool::clear));
}

void HistoryBigram::forget(std::string_view word) {
    FCITX_D();
    boost::range::for_each(d->pools_,
                           [word](auto &pool) { pool.forget(word); });
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
    lookup += "|";
    boost::range::for_each(
        d->pools_, [&words, lookup, maxSize](const HistoryBigramPool &pool) {
            pool.fillPredict(words, lookup, maxSize);
        });
}
} // namespace libime
