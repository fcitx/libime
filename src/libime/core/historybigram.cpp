/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#include "historybigram.h"
#include "constants.h"
#include "datrie.h"
#include "utils.h"
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <cmath>
#include <fcitx-utils/log.h>

namespace libime {

struct WeightedTrie {
    using TrieType = DATrie<int32_t>;

public:
    WeightedTrie() = default;

    void load(std::istream &in) {
        TrieType trie;
        trie.load(in);
        weightedSize_ = 0;
        trie_ = std::move(trie);
        trie_.foreach([this](int32_t value, size_t, uint64_t) {
            weightedSize_ += value;
            return true;
        });
    }

    void save(std::ostream &out) { trie_.save(out); }

    void clear() { trie_.clear(); }

    const TrieType &trie() const { return trie_; }

    int32_t weightedSize() const { return weightedSize_; }

    int32_t freq(boost::string_view s) const {
        auto v = trie_.exactMatchSearch(s.data(), s.size());
        if (trie_.isNoValue(v)) {
            return 0;
        }
        return v;
    }

    void incFreq(boost::string_view s, int32_t delta) {
        trie_.update(s.data(), s.size(),
                     [delta](int32_t v) { return v + delta; });
        weightedSize_ += delta;
    }

    void decFreq(boost::string_view s, int32_t delta) {
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

    void eraseByKey(boost::string_view s) {
        auto v = trie_.exactMatchSearch(s.data(), s.size());
        if (trie_.isNoValue(v)) {
            return;
        }
        trie_.erase(s);
        decWeightedSize(v);
    }

    void eraseByPrefix(boost::string_view s) {
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

    void eraseBySuffix(boost::string_view s) {
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

constexpr const static float decay = 1.0f;
class HistoryBigramPool {
public:
    HistoryBigramPool(size_t maxSize = 0, HistoryBigramPool *next = nullptr)
        : maxSize_(maxSize), next_(next) {
        if (maxSize) {
            assert(next);
        } else {
            assert(!next);
        }
    }

    void setUnknownPenalty(float unknown) {
        unknown_ = unknown;
        if (next_) {
            next_->setUnknownPenalty(unknown);
        }
    }

    void setPenaltyFactor(float factor) {
        penaltyFactor_ = factor;
        if (next_) {
            next_->setPenaltyFactor(factor);
        }
    }

    void load(std::istream &in) {
        clear();
        if (maxSize_) {
            uint32_t count;
            throw_if_io_fail(unmarshall(in, count));
            while (count--) {
                uint32_t size;
                throw_if_io_fail(unmarshall(in, size));
                std::vector<std::string> sentence;
                while (size--) {
                    std::string buffer;
                    throw_if_io_fail(unmarshallString(in, buffer));
                    sentence.emplace_back(std::move(buffer));
                }
                add(sentence);
            }
            next_->load(in);
        } else {
            unigram_.load(in);
            bigram_.load(in);
        }
    }

    void save(std::ostream &out) {
        if (maxSize_) {
            uint32_t count = recent_.size();
            throw_if_io_fail(marshall(out, count));
            for (auto &sentence : recent_) {
                uint32_t size = sentence.size();
                throw_if_io_fail(marshall(out, size));
                for (auto &s : sentence) {
                    throw_if_io_fail(marshallString(out, s));
                }
            }
            next_->save(out);
        } else {
            unigram_.save(out);
            bigram_.save(out);
        }
    }

    void dump(std::ostream &out) {
        if (maxSize_) {
            for (auto &sentence : recent_ | boost::adaptors::reversed) {
                bool first = true;
                for (auto &s : sentence) {
                    if (first) {
                        first = false;
                    } else {
                        std::cout << " ";
                    }
                    std::cout << s;
                }
                std::cout << std::endl;
            }
            next_->dump(out);
        } else {
            unigram_.trie().foreach(
                [this, &out](int32_t value, size_t _len,
                             DATrie<int32_t>::position_type pos) {
                    std::string buf;
                    unigram_.trie().suffix(buf, _len, pos);
                    out << buf << " " << value << std::endl;
                    return true;
                });
            bigram_.trie().foreach(
                [this, &out](int32_t value, size_t _len,
                             DATrie<int32_t>::position_type pos) {
                    std::string buf;
                    bigram_.trie().suffix(buf, _len, pos);
                    out << buf << " " << value << std::endl;
                    return true;
                });
        }
    }

    void clear() {
        recent_.clear();
        unigram_.clear();
        bigram_.clear();
        size_ = 0;
        if (next_) {
            next_->clear();
        }
    }

    template <typename R>
    void add(const R &sentence) {
        if (sentence.empty()) {
            return;
        }
        if (maxSize_) {
            while (recent_.size() >= maxSize_) {
                next_->add(recent_.back());
                remove(recent_.back());
                recent_.pop_back();
            }
        }

        std::vector<std::string> newSentence;
        auto delta = weightForSentence(sentence);
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
        if (maxSize_) {
            recent_.push_front(std::move(newSentence));
        }
        unigram_.incFreq("<s>", delta);
        unigram_.incFreq("</s>", delta);
        incBigram("<s>", sentence.front(), delta);
        incBigram(sentence.back(), "</s>", delta);
    }

    float unigramFreq(boost::string_view s) const {
        auto v = unigram_.freq(s);
        if (next_) {
            return v + decay * next_->unigramFreq(s);
        }
        return v;
    }

    float bigramFreq(boost::string_view s1, boost::string_view s2) const {
        std::string s;
        s.append(s1.data(), s1.size());
        s += '|';
        s.append(s2.data(), s2.size());
        auto v = bigram_.freq(s);
        if (next_) {
            v += decay * next_->bigramFreq(s1, s2);
        }
        return v;
    }

    bool isUnknown(boost::string_view word) const {
        return unigramFreq(word) == 0 && (!next_ || next_->isUnknown(word));
    }

    float score(boost::string_view prev, boost::string_view cur) const {
        int uf0 = unigramFreq(prev);
        int bf = bigramFreq(prev, cur);
        int uf1 = unigramFreq(cur);

        float bigramWeight = 0.68f;
        // add 0.5 to avoid div 0
        float pr = 0.0f;
        pr += bigramWeight * float(bf) / float(uf0 + 0.5f);
        pr += (1.0f - bigramWeight) * float(uf1) / float(unigramSize() + 0.5f);
        if (auto sizeDiff = (maxSize() - realSize())) {
            auto penalty = 1 + (penaltyFactor_ * sizeDiff / maxSize());
            if (penalty > 1) {
                pr /= penalty;
            }
        }

        if (pr >= 1.0) {
            pr = 1.0;
        }
        if (pr == 0) {
            pr = unknown_;
        }

        return pr;
    }

    size_t maxSize() const {
        return (maxSize_ ? maxSize_ : 0) + (next_ ? next_->maxSize() : 0);
    }

    size_t realSize() const {
        return recent_.size() + (next_ ? next_->realSize() : 0);
    }

    size_t unigramSize() const {
        auto currentSize =
            std::max(static_cast<size_t>(
                         maxSize_ * DEFAULT_USER_LANGUAGE_MODEL_UNIGRAM_WEIGHT),
                     static_cast<size_t>(unigram_.weightedSize()));
        if (next_) {
            currentSize += next_->unigramSize();
        }
        return currentSize;
    }

    void forget(boost::string_view word) {
        if (maxSize_) {
            auto iter = recent_.begin();
            while (iter != recent_.end()) {
                if (std::find(iter->begin(), iter->end(), word) !=
                    iter->end()) {
                    remove(*iter);
                    iter = recent_.erase(iter);
                } else {
                    ++iter;
                }
            }
        } else {
            unigram_.decFreq(word, unigram_.freq(word));
            std::string prefix = word.to_string() + '|';
            std::string suffix = '|' + word.to_string();
        }

        if (next_) {
            next_->forget(word);
        }
    }

private:
    template <typename R>
    int32_t weightForSentence(const R &sentence) {
        // Short <= 2 unigram weight.
        if (sentence.size() <= 2) {
            return DEFAULT_USER_LANGUAGE_MODEL_UNIGRAM_WEIGHT;
        }

        if (sentence.size() < DEFAULT_USER_LANGUAGE_MODEL_UNIGRAM_WEIGHT) {
            return sentence.size();
        }
        return DEFAULT_USER_LANGUAGE_MODEL_BIGRAM_WEIGHT;
    }

    template <typename R>
    void remove(const R &sentence) {
        int32_t delta = weightForSentence(sentence);
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

    void decBigram(boost::string_view s1, boost::string_view s2,
                   int32_t delta) {
        std::string ss;
        ss.append(s1.data(), s1.size());
        ss += '|';
        ss.append(s2.data(), s2.size());
        bigram_.decFreq(ss, delta);
    }

    void incBigram(boost::string_view s1, boost::string_view s2, int delta) {
        std::string ss;
        ss.append(s1.data(), s1.size());
        ss += '|';
        ss.append(s2.data(), s2.size());
        bigram_.incFreq(ss, delta);
    }

    const size_t maxSize_;
    float unknown_ = DEFAULT_LANGUAGE_MODEL_UNKNOWN_PROBABILITY_PENALTY;
    float penaltyFactor_ = DEFAULT_USER_LANGUAGE_MODEL_PENALTY_FACTOR;

    // Used when maxSize_ != 0.
    size_t size_ = 0;
    std::list<std::vector<std::string>> recent_;

    // Used for look up
    WeightedTrie unigram_;
    WeightedTrie bigram_;

    HistoryBigramPool *next_;
};

class HistoryBigramPrivate {
public:
    HistoryBigramPrivate() {}

    // A probabilty, not log'ed
    float unknown_ = DEFAULT_LANGUAGE_MODEL_UNKNOWN_PROBABILITY_PENALTY;
    float penaltyFactor_ = DEFAULT_USER_LANGUAGE_MODEL_PENALTY_FACTOR;
    HistoryBigramPool finalPool_;
    HistoryBigramPool middlePool_{256, &finalPool_};
    HistoryBigramPool recentPool_{64, &middlePool_};
};

HistoryBigram::HistoryBigram()
    : d_ptr(std::make_unique<HistoryBigramPrivate>()) {
    setUnknownPenalty(
        std::log10(DEFAULT_LANGUAGE_MODEL_UNKNOWN_PROBABILITY_PENALTY));
    setPenaltyFactor(DEFAULT_USER_LANGUAGE_MODEL_PENALTY_FACTOR);
}

FCITX_DEFINE_DEFAULT_DTOR_AND_MOVE(HistoryBigram)

void HistoryBigram::setUnknownPenalty(float unknown) {
    FCITX_D();
    d->unknown_ = unknown;
    d->recentPool_.setUnknownPenalty(std::pow(10, unknown));
}

float HistoryBigram::unknownPenalty() const {
    FCITX_D();
    return d->unknown_;
}

void HistoryBigram::setPenaltyFactor(float factor) {
    FCITX_D();
    d->penaltyFactor_ = factor;
    d->recentPool_.setPenaltyFactor(factor);
}

float HistoryBigram::penaltyFactor() const {
    FCITX_D();
    return d->penaltyFactor_;
}

void HistoryBigram::add(const libime::SentenceResult &sentence) {
    FCITX_D();
    d->recentPool_.add(sentence.sentence() |
                       boost::adaptors::transformed(
                           [](const auto &item) { return item->word(); }));
}

void HistoryBigram::add(const std::vector<std::string> &sentence) {
    FCITX_D();
    d->recentPool_.add(sentence);
}

bool HistoryBigram::isUnknown(boost::string_view v) const {
    FCITX_D();
    return d->recentPool_.isUnknown(v);
}

float HistoryBigram::score(boost::string_view prev,
                           boost::string_view cur) const {
    FCITX_D();
    if (prev.empty()) {
        prev = "<s>";
    }
    if (cur.empty()) {
        cur = "</s>";
    }
    auto pr = d->recentPool_.score(prev, cur);
    return std::log10(pr);
}

void HistoryBigram::load(std::istream &in) {
    FCITX_D();
    d->recentPool_.load(in);
}

void HistoryBigram::save(std::ostream &out) {
    FCITX_D();
    d->recentPool_.save(out);
}

void HistoryBigram::dump(std::ostream &out) {
    FCITX_D();
    d->recentPool_.dump(out);
}

void HistoryBigram::clear() {
    FCITX_D();
    d->recentPool_.clear();
}

void HistoryBigram::forget(boost::string_view word) {
    FCITX_D();
    d->recentPool_.forget(word);
}
}
