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
#include "pinyincontext.h"
#include "libime/core/historybigram.h"
#include "libime/core/userlanguagemodel.h"
#include "libime/pinyin/constants.h"
#include "pinyindecoder.h"
#include "pinyinencoder.h"
#include "pinyinime.h"
#include "pinyinmatchstate.h"
#include <algorithm>
#include <fcitx-utils/log.h>
#include <iostream>

namespace libime {

struct SelectedPinyin {
    SelectedPinyin(size_t s, WordNode word, std::string encodedPinyin)
        : offset_(s), word_(std::move(word)),
          encodedPinyin_(std::move(encodedPinyin)) {}
    size_t offset_;
    WordNode word_;
    std::string encodedPinyin_;
};

class PinyinContextPrivate {
public:
    PinyinContextPrivate(PinyinContext *q, PinyinIME *ime)
        : ime_(ime), matchState_(q) {}

    std::vector<std::vector<SelectedPinyin>> selected_;

    bool sp_ = false;
    PinyinIME *ime_;
    SegmentGraph segs_;
    Lattice lattice_;
    PinyinMatchState matchState_;
    std::vector<SentenceResult> candidates_;
    std::vector<fcitx::ScopedConnection> conn_;
};

PinyinContext::PinyinContext(PinyinIME *ime)
    : InputBuffer(fcitx::InputBufferOption::AsciiOnly),
      d_ptr(std::make_unique<PinyinContextPrivate>(this, ime)) {
    FCITX_D();
    d->conn_.emplace_back(
        ime->connect<PinyinIME::optionChanged>([this]() { clear(); }));
    d->conn_.emplace_back(
        ime->dict()->connect<PinyinDictionary::dictionaryChanged>(
            [this](size_t) {
                FCITX_D();
                d->matchState_.clear();
            }));
}

PinyinContext::~PinyinContext() {}

void PinyinContext::setUseShuangpin(bool sp) {
    FCITX_D();
    d->sp_ = sp;
    d->matchState_.clear();
}

bool PinyinContext::useShuangpin() const {
    FCITX_D();
    return d->sp_;
}

void PinyinContext::typeImpl(const char *s, size_t length) {
    cancelTill(cursor());
    InputBuffer::typeImpl(s, length);
    update();
}

void PinyinContext::erase(size_t from, size_t to) {
    if (from == to) {
        return;
    }

    // check if erase everything
    if (from == 0 && to >= size()) {
        FCITX_D();
        d->candidates_.clear();
        d->selected_.clear();
        d->lattice_.clear();
        d->matchState_.clear();
        d->segs_ = SegmentGraph();
    } else {
        cancelTill(from);
    }
    InputBuffer::erase(from, to);

    if (size()) {
        update();
    }
}

void PinyinContext::setCursor(size_t pos) {
    auto cancelled = cancelTill(pos);
    InputBuffer::setCursor(pos);
    if (cancelled) {
        update();
    }
}

int PinyinContext::pinyinBeforeCursor() const {
    FCITX_D();
    auto c = cursor() + selectedLength();
    if (d->candidates_.size()) {
        for (auto &s : d->candidates_[0].sentence()) {
            for (auto iter = s->path().begin(),
                      end = std::prev(s->path().end());
                 iter < end; iter++) {
                auto from = (*iter)->index(), to = (*std::next(iter))->index();
                if (to >= c) {
                    return from;
                }
            }
        }
    }
    return -1;
}

int PinyinContext::pinyinAfterCursor() const {
    FCITX_D();
    auto c = cursor() + selectedLength();
    if (d->candidates_.size()) {
        for (auto &s : d->candidates_[0].sentence()) {
            for (auto iter = s->path().begin(),
                      end = std::prev(s->path().end());
                 iter < end; iter++) {
                auto to = (*std::next(iter))->index();
                if (to > c) {
                    return to;
                }
            }
        }
    }
    return -1;
}

const std::vector<SentenceResult> &PinyinContext::candidates() const {
    FCITX_D();
    return d->candidates_;
}

void PinyinContext::select(size_t idx) {
    FCITX_D();
    assert(idx < d->candidates_.size());

    auto offset = selectedLength();

    d->selected_.emplace_back();

    auto &selection = d->selected_.back();
    for (auto &p : d->candidates_[idx].sentence()) {
        selection.emplace_back(
            offset + p->to()->index(),
            WordNode{p->word(), d->ime_->model()->index(p->word())},
            static_cast<const PinyinLatticeNode *>(p)->encodedPinyin());
    }
    // add some special code for handling separator at the end
    auto remain = boost::string_view(userInput()).substr(selectedLength());
    if (!remain.empty()) {
        if (std::all_of(remain.begin(), remain.end(),
                        [](char c) { return c == '\''; })) {
            selection.emplace_back(size(), WordNode("", 0), "");
        }
    }

    update();
}

bool PinyinContext::cancelTill(size_t pos) {
    bool cancelled = false;
    while (selectedLength() > pos) {
        cancel();
        cancelled = true;
    }
    return cancelled;
}

void PinyinContext::cancel() {
    FCITX_D();
    if (d->selected_.size()) {
        d->selected_.pop_back();
    }
    update();
}

State PinyinContext::state() const {
    FCITX_D();
    auto model = d->ime_->model();
    State state = model->nullState();
    if (d->selected_.size()) {
        for (auto &s : d->selected_) {
            for (auto &item : s) {
                if (item.word_.word().empty()) {
                    continue;
                }
                State temp;
                model->score(state, item.word_, temp);
                state = std::move(temp);
            }
        }
    }
    return state;
}

void PinyinContext::update() {
    FCITX_D();
    if (size() == 0) {
        clear();
        return;
    }

    if (selected()) {
        d->candidates_.clear();
    } else {
        size_t start = 0;
        auto model = d->ime_->model();
        State state = model->nullState();
        if (d->selected_.size()) {
            start = d->selected_.back().back().offset_;

            for (auto &s : d->selected_) {
                for (auto &item : s) {
                    if (item.word_.word().empty()) {
                        continue;
                    }
                    State temp;
                    model->score(state, item.word_, temp);
                    state = std::move(temp);
                }
            }
        }
        SegmentGraph newGraph;
        if (auto spProfile = d->matchState_.shuangpinProfile()) {
            newGraph = PinyinEncoder::parseUserShuangpin(
                boost::string_view(userInput()).substr(start), *spProfile,
                d->ime_->fuzzyFlags());
        } else {
            newGraph = PinyinEncoder::parseUserPinyin(
                boost::string_view(userInput()).substr(start),
                d->ime_->fuzzyFlags());
        }
        d->segs_.merge(
            newGraph,
            [d](const std::unordered_set<const SegmentGraphNode *> &nodes) {
                d->lattice_.discardNode(nodes);
                d->matchState_.discardNode(nodes);
            });
        auto &graph = d->segs_;

        d->ime_->decoder()->decode(d->lattice_, d->segs_, d->ime_->nbest(),
                                   state, d->ime_->maxDistance(),
                                   d->ime_->minPath(), d->ime_->beamSize(),
                                   d->ime_->frameSize(), &d->matchState_);

        d->candidates_.clear();
        std::unordered_set<std::string> dup;
        for (size_t i = 0, e = d->lattice_.sentenceSize(); i < e; i++) {
            d->candidates_.push_back(d->lattice_.sentence(i));
            dup.insert(d->candidates_.back().toString());
        }

        auto bos = &graph.start();

        auto beginSize = d->candidates_.size();
        for (size_t i = graph.size(); i > 0; i--) {
            float min = 0;
            float max = -std::numeric_limits<float>::max();
            auto distancePenalty = d->ime_->model()->unknownPenalty() /
                                   PINYIN_DISTANCE_PENALTY_FACTOR;
            for (auto &graphNode : graph.nodes(i)) {
                auto distance = graph.distanceToEnd(graphNode);
                auto adjust = static_cast<float>(distance) * distancePenalty;
                for (auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                    if (latticeNode.from() == bos) {
                        if (!d->ime_->model()->isNodeUnknown(latticeNode)) {
                            if (latticeNode.score() < min) {
                                min = latticeNode.score();
                            }
                            if (latticeNode.score() > max) {
                                max = latticeNode.score();
                            }
                        }
                        if (dup.count(latticeNode.word())) {
                            continue;
                        }
                        d->candidates_.push_back(
                            latticeNode.toSentenceResult(adjust));
                        dup.insert(latticeNode.word());
                    }
                }
            }
            for (auto &graphNode : graph.nodes(i)) {
                auto distance = graph.distanceToEnd(graphNode);
                auto adjust = static_cast<float>(distance) * distancePenalty;
                for (auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                    if (latticeNode.from() != bos &&
                        latticeNode.score() > min &&
                        latticeNode.score() + d->ime_->maxDistance() > max) {
                        auto fullWord = latticeNode.fullWord();
                        if (dup.count(fullWord)) {
                            continue;
                        }
                        d->candidates_.push_back(
                            latticeNode.toSentenceResult(adjust));
                    }
                }
            }
        }
        std::sort(d->candidates_.begin() + beginSize, d->candidates_.end(),
                  std::greater<SentenceResult>());
    }

    if (cursor() < selectedLength()) {
        setCursor(selectedLength());
    }
}

bool PinyinContext::selected() const {
    FCITX_D();
    if (userInput().empty()) {
        return false;
    }

    if (d->selected_.size()) {
        if (d->selected_.back().back().offset_ == size()) {
            return true;
        }
    }

    return false;
}

std::string PinyinContext::selectedSentence() const {
    FCITX_D();
    std::string ss;
    for (auto &s : d->selected_) {
        for (auto &item : s) {
            ss += item.word_.word();
        }
    }
    return ss;
}

size_t PinyinContext::selectedLength() const {
    FCITX_D();
    if (d->selected_.size()) {
        return d->selected_.back().back().offset_;
    }
    return 0;
}

std::string PinyinContext::preedit() const { return preeditWithCursor().first; }

std::pair<std::string, size_t> PinyinContext::preeditWithCursor() const {
    FCITX_D();
    std::string ss = selectedSentence();
    auto len = selectedLength();
    auto c = cursor();
    size_t actualCursor = ss.size();
    // should not happen
    if (c < len) {
        c = len;
    }

    auto resultSize = ss.size();

    if (d->candidates_.size()) {
        bool first = true;
        for (auto &s : d->candidates_[0].sentence()) {
            for (auto iter = s->path().begin(),
                      end = std::prev(s->path().end());
                 iter < end; iter++) {
                if (!first) {
                    ss += " ";
                    resultSize += 1;
                } else {
                    first = false;
                }
                auto from = (*iter)->index(), to = (*std::next(iter))->index();
                if (c >= from + len && c < to + len) {
                    actualCursor = resultSize + c - from - len;
                }
                auto pinyin = d->segs_.segment(from, to);
                ss.append(pinyin.data(), pinyin.size());
                resultSize += pinyin.size();
            }
        }
    }
    if (c == size()) {
        actualCursor = resultSize;
    }
    return {ss, actualCursor};
}

std::vector<std::string> PinyinContext::selectedWords() const {
    FCITX_D();
    std::vector<std::string> newSentence;
    for (auto &s : d->selected_) {
        for (auto &item : s) {
            if (!item.word_.word().empty()) {
                newSentence.push_back(item.word_.word());
            }
        }
    }
    return newSentence;
}

std::string PinyinContext::selectedFullPinyin() const {
    FCITX_D();
    std::string pinyin;
    for (auto &s : d->selected_) {
        for (auto &item : s) {
            if (!item.word_.word().empty()) {
                if (!pinyin.empty()) {
                    pinyin.push_back('\'');
                }
                pinyin += PinyinEncoder::decodeFullPinyin(item.encodedPinyin_);
            }
        }
    }
    return pinyin;
}

std::string PinyinContext::candidateFullPinyin(size_t idx) const {
    FCITX_D();
    std::string pinyin;
    for (auto &p : d->candidates_[idx].sentence()) {
        if (!p->word().empty()) {
            if (!pinyin.empty()) {
                pinyin.push_back('\'');
            }
            pinyin += PinyinEncoder::decodeFullPinyin(
                static_cast<const PinyinLatticeNode *>(p)->encodedPinyin());
        }
    }
    return pinyin;
}

void PinyinContext::learn() {
    FCITX_D();
    if (!selected()) {
        return;
    }

    if (learnWord()) {
        std::vector<std::string> newSentence{sentence()};
        d->ime_->model()->history().add(newSentence);
    } else {
        std::vector<std::string> newSentence;
        for (auto &s : d->selected_) {
            for (auto &item : s) {
                if (!item.word_.word().empty()) {
                    newSentence.push_back(item.word_.word());
                }
            }
        }
        d->ime_->model()->history().add(newSentence);
    }
}

bool PinyinContext::learnWord() {
    FCITX_D();
    std::string ss;
    std::string pinyin;
    if (d->selected_.empty()) {
        return false;
    }
    // don't learn single character.
    if (d->selected_.size() == 1 && d->selected_[0].size() == 1) {
        return false;
    }
    for (auto &s : d->selected_) {
        bool first = true;
        for (auto &item : s) {
            if (!item.word_.word().empty()) {
                if (item.encodedPinyin_.size() != 2) {
                    return false;
                }
                if (first) {
                    first = false;
                    ss += item.word_.word();
                    if (!pinyin.empty()) {
                        pinyin.push_back('\'');
                    }
                    pinyin +=
                        PinyinEncoder::decodeFullPinyin(item.encodedPinyin_);
                } else {
                    return false;
                }
            }
        }
    }

    d->ime_->dict()->addWord(PinyinDictionary::UserDict, pinyin, ss);

    return true;
}

PinyinIME *PinyinContext::ime() const {
    FCITX_D();
    return d->ime_;
}
}
