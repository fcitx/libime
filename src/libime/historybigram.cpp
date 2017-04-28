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
#include "utils.h"
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <cmath>

namespace libime {

constexpr const static float decay = 0.8f;
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

    void setUnknown(float unknown) {
        unknown_ = unknown;
        if (next_) {
            next_->setUnknown(unknown);
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
                    uint32_t length;
                    throw_if_io_fail(unmarshall(in, length));
                    std::vector<char> buffer;
                    buffer.resize(length);
                    throw_if_io_fail(
                        in.read(buffer.data(), sizeof(char) * length));
                    sentence.emplace_back(buffer.begin(), buffer.end());
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
            for (auto &sentence : recent_ | boost::adaptors::reversed) {
                uint32_t size = sentence.size();
                throw_if_io_fail(marshall(out, size));
                for (auto &s : sentence) {
                    uint32_t length = s.size();
                    throw_if_io_fail(marshall(out, length));
                    throw_if_io_fail(out.write(s.data(), s.size()));
                }
            }
            next_->save(out);
        } else {
            unigram_.save(out);
            bigram_.save(out);
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
        incUnigram("<s>");
        incUnigram("</s>");
        incBigram("<s>", sentence.front());
        incBigram(sentence.back(), "</s>");
        recent_.push_front(std::move(newSentence));
        size_++;
    }

    float unigramFreq(boost::string_view s) const {
        auto v = unigram_.exactMatchSearch(s.data(), s.size());
        if (v == unigram_.NO_VALUE) {
            return 0;
        }
        if (next_) {
            return v + decay * next_->unigramFreq(s);
        }
        return v;
    }

    float bigramFreq(boost::string_view s1, boost::string_view s2) const {
        std::stringstream ss;
        ss << s1 << "|" << s2;
        auto s = ss.str();
        auto v = bigram_.exactMatchSearch(s.c_str(), s.size());
        if (v == bigram_.NO_VALUE) {
            return 0;
        }
        if (next_) {
            return v + decay * next_->bigramFreq(s1, s2);
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

        // add 0.5 to avoid div 0
        float pr = 0.0f;
        pr += 0.68f * float(bf) / float(uf0 + 0.5f);
        pr += 0.32f * float(uf1) / float(size() + 0.5f);

        if (pr >= 1.0) {
            pr = 1.0;
        }
        if (pr == 0) {
            pr = unknown_;
        }
        if (next_) {
            return pr + decay * next_->score(prev, cur) / (1 + decay);
        }
        return pr;
    }

    size_t size() const {
        return maxSize_ ? (size_ + (maxSize_ - size_) / 10) : size_;
    }

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
        decBigram("<s>", sentence.front());
        decBigram(sentence.back(), "</s>");
        size_--;
    }

    static void decFreq(boost::string_view s, DATrie<int32_t> &trie) {
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
    }

    static void incFreq(boost::string_view s, DATrie<int32_t> &trie) {
        trie.update(s.data(), s.size(), [](int32_t v) { return v + 1; });
    }

    void decUnigram(boost::string_view s) { decFreq(s, unigram_); }

    void incUnigram(boost::string_view s) { incFreq(s, unigram_); }

    void decBigram(boost::string_view s1, boost::string_view s2) {
        std::stringstream ss;
        ss << s1 << "|" << s2;
        decFreq(ss.str(), bigram_);
    }

    void incBigram(boost::string_view s1, boost::string_view s2) {
        std::stringstream ss;
        ss << s1 << "|" << s2;
        incFreq(ss.str(), bigram_);
    }

    size_t maxSize_;
    float unknown_ = 1 / 20000.0f;
    size_t size_ = 0;
    std::list<std::vector<std::string>> recent_;
    DATrie<int32_t> unigram_;
    DATrie<int32_t> bigram_;
    HistoryBigramPool *next_;
};

class HistoryBigramPrivate {
public:
    HistoryBigramPrivate() {}

    HistoryBigramPool finalPool_;
    HistoryBigramPool middlePool_{512, &finalPool_};
    HistoryBigramPool recentPool_{128, &middlePool_};
};

HistoryBigram::HistoryBigram()
    : d_ptr(std::make_unique<HistoryBigramPrivate>()) {}

HistoryBigram::~HistoryBigram() {}

void HistoryBigram::setUnknown(float unknown) {
    FCITX_D();
    d->recentPool_.setUnknown(std::pow(10, unknown));
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

void HistoryBigram::clear() {
    FCITX_D();
    d->recentPool_.clear();
}
}
