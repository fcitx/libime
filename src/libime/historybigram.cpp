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
#include "historybigram.h"
#include "datrie.h"
#include <boost/range/adaptor/transformed.hpp>
#include <cmath>

namespace libime {

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
        for (auto iter = sentence.begin(), end = sentence.end(); iter != end;
             iter++) {
            incUnigram(*iter);
            auto next = std::next(iter);
            if (next != end) {
                incBigram(*iter, *next);
            }
            std::stringstream ss;
            ss << *iter;
            newSentence.push_back(ss.str());
        }
        recent_.push_front(std::move(newSentence));
        size_++;
    }

    int unigramFreq(boost::string_view s) const {
        auto v = unigram_.exactMatchSearch(s.data(), s.size());
        if (v == unigram_.NO_VALUE) {
            return 0;
        }
        return v;
    }

    int bigramFreq(boost::string_view s1, boost::string_view s2) const {
        std::stringstream ss;
        ss << s1 << "|" << s2;
        auto s = ss.str();
        auto v = bigram_.exactMatchSearch(s.c_str(), s.size());
        if (v == bigram_.NO_VALUE) {
            return 0;
        }
        return v;
    }

    size_t size() const { return size_; }

private:
    template <typename R>
    void remove(const R &sentence) {
        for (auto iter = sentence.begin(), end = sentence.end(); iter != end;
             iter++) {
            decUnigram(*iter);
            auto next = std::next(iter);
            if (next != end) {
                decBigram(*iter, *next);
            }
        }
        size_--;
    }

    static void decFreq(boost::string_view s, DATrie<int32_t> &trie,
                        size_t &totalSize) {
        auto v = trie.exactMatchSearch(s.data(), s.size());
        if (v == trie.NO_VALUE) {
            return;
        }
        v -= 1;
        if (v <= 0) {
            trie.erase(s.data(), s.size());
        } else {
            trie.set(s.data(), s.size(), v);
        }
        totalSize -= 1;
    }

    static void incFreq(boost::string_view s, DATrie<int32_t> &trie,
                        size_t &totalSize) {
        trie.update(s.data(), s.size(), [](int32_t v) { return v + 1; });
        totalSize += 1;
    }

    void decUnigram(boost::string_view s) {
        decFreq(s, unigram_, unigramTotal_);
    }

    void incUnigram(boost::string_view s) {
        incFreq(s, unigram_, unigramTotal_);
    }

    void decBigram(boost::string_view s1, boost::string_view s2) {
        std::stringstream ss;
        ss << s1 << "|" << s2;
        decFreq(ss.str(), bigram_, bigramTotal_);
    }

    void incBigram(boost::string_view s1, boost::string_view s2) {
        std::stringstream ss;
        ss << s1 << "|" << s2;
        incFreq(ss.str(), bigram_, bigramTotal_);
    }

    DATrie<int32_t> unigram_;
    size_t unigramTotal_ = 0;
    DATrie<int32_t> bigram_;
    size_t bigramTotal_ = 0;

    size_t maxSize_;
    size_t size_ = 0;
    HistoryBigramPool *next_;
    std::list<std::vector<std::string>> recent_;
};

class HistoryBigramPrivate {
public:
    constexpr const static float decay = 0.05f;

    HistoryBigramPrivate() {}

    HistoryBigramPool finalPool;
    HistoryBigramPool recentPool{8192, &finalPool};
    float unknown_ = -5;

    float unigramFreq(boost::string_view s) const {
        return recentPool.unigramFreq(s) + finalPool.unigramFreq(s) * decay;
    }

    float bigramFreq(boost::string_view s1, boost::string_view s2) const {
        return recentPool.bigramFreq(s1, s2) +
               finalPool.bigramFreq(s1, s2) * decay;
    }

    float size() const { return recentPool.size() + finalPool.size() * decay; }
};

HistoryBigram::HistoryBigram()
    : d_ptr(std::make_unique<HistoryBigramPrivate>()) {}

HistoryBigram::~HistoryBigram() {}

void HistoryBigram::setUnknown(float unknown) {
    FCITX_D();
    d->unknown_ = unknown;
}

void HistoryBigram::add(const libime::SentenceResult &sentence) {
    FCITX_D();
    d->recentPool.add(sentence.sentence() |
                      boost::adaptors::transformed(
                          [](const auto &item) { return item->word(); }));
}

void HistoryBigram::add(const std::vector<std::string> &sentence) {
    FCITX_D();
    d->recentPool.add(sentence);
}

bool HistoryBigram::isUnknown(boost::string_view v) const {
    FCITX_D();
    return v.empty() || d->unigramFreq(v) == 0;
}

float HistoryBigram::score(boost::string_view prev,
                           boost::string_view cur) const {
    FCITX_D();
    int uf0 = d->unigramFreq(prev);
    int bf = d->bigramFreq(prev, cur);
    int uf1 = d->unigramFreq(cur);

    // add 0.5 to avoid div 0
    float pr = 0.0f;
    pr += 0.68f * float(bf) / float(uf0 + 0.5f);
    pr += 0.32f * float(uf1) / float(d->size() + 0.5f);

    if (pr >= 1.0) {
        return 0;
    }
    if (pr == 0) {
        return d->unknown_;
    }
    return std::log10(pr);
}

void HistoryBigram::load(std::istream &) {}

void HistoryBigram::save(std::ostream &) {}
}
