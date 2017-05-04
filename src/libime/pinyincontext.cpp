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
#include "pinyincontext.h"
#include "historybigram.h"
#include "pinyindecoder.h"
#include "pinyinencoder.h"
#include "pinyinime.h"
#include "userlanguagemodel.h"
#include <algorithm>
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
    PinyinContextPrivate(PinyinIME *ime) : ime_(ime) {}

    std::vector<std::pair<bool, std::vector<SelectedPinyin>>> selected;

    PinyinIME *ime_;
    SegmentGraph segs_;
    Lattice lattice_;
    PinyinMatchState matchState_;
    std::vector<SentenceResult> candidates_;
};

PinyinContext::PinyinContext(PinyinIME *ime)
    : InputBuffer(true), d_ptr(std::make_unique<PinyinContextPrivate>(ime)) {}

PinyinContext::~PinyinContext() {}

void PinyinContext::type(boost::string_view s) {
    cancelTill(cursor());
    InputBuffer::type(s);
    update();
}

void PinyinContext::erase(size_t from, size_t to) {
    if (from == to) {
        return;
    }
    cancelTill(from);
    InputBuffer::erase(from, to);
    update();
}

void PinyinContext::setCursor(size_t pos) {
    auto cancelled = cancelTill(pos);
    InputBuffer::setCursor(pos);
    if (cancelled) {
        update();
    }
}

void PinyinContext::clear() {
    FCITX_D();
    d->candidates_.clear();
    d->selected.clear();
    d->lattice_.clear();
    d->matchState_.clear();
    d->segs_ = SegmentGraph();
    InputBuffer::clear();
}

const std::vector<SentenceResult> &PinyinContext::candidates() const {
    FCITX_D();
    return d->candidates_;
}

void PinyinContext::select(size_t idx) {
    FCITX_D();
    assert(idx < d->candidates_.size());

    auto offset = selectedLength();

    d->selected.emplace_back();

    auto &selection = d->selected.back();
    if (idx != 0) {
        selection.first = true;
    }
    for (auto &p : d->candidates_[idx].sentence()) {
        selection.second.emplace_back(
            offset + p->to()->index(),
            WordNode{p->word(), d->ime_->model()->index(p->word())},
            static_cast<const PinyinLatticeNode *>(p)->encodedPinyin());
    }
    // add some special code for handling separator at the end
    auto remain = boost::string_view(userInput()).substr(selectedLength());
    if (!remain.empty()) {
        if (std::all_of(remain.begin(), remain.end(),
                        [](char c) { return c == '\''; })) {
            selection.second.emplace_back(size(), WordNode("", 0), "");
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
    if (d->selected.size()) {
        d->selected.pop_back();
    }
    update();
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
        if (d->selected.size()) {
            start = d->selected.back().second.back().offset_;

            for (auto &s : d->selected) {
                for (auto &item : s.second) {
                    if (item.word_.word().empty()) {
                        continue;
                    }
                    State temp;
                    model->score(state, item.word_, temp);
                    state = std::move(temp);
                }
            }
        }
        auto newGraph = PinyinEncoder::parseUserPinyin(
            boost::string_view(userInput()).substr(start),
            d->ime_->fuzzyFlags());
        d->segs_.merge(newGraph, [d] (const std::unordered_set<const SegmentGraphNode *> &nodes) {
            d->lattice_.discardNode(nodes);
            d->matchState_.discardNode(nodes);
        });
        auto &graph = d->segs_;

        d->ime_->decoder()->decode(d->lattice_, d->segs_, d->ime_->nbest(),
                                   state, d->ime_->maxDistance(),
                                   d->ime_->minPath(), d->ime_->beamSize(),
                                   d->ime_->frameSize());

        d->candidates_.clear();
        std::unordered_set<std::string> dup;
        for (size_t i = 0, e = d->lattice_.sentenceSize(); i < e; i++) {
            d->candidates_.push_back(d->lattice_.sentence(i));
            dup.insert(d->candidates_.back().toString());
        }

        auto bos = &graph.start();

        for (size_t i = graph.size(); i > 0; i--) {
            float min = 0;
            auto beginSize = d->candidates_.size();
            for (auto &graphNode : graph.nodes(i)) {
                for (auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                    if (latticeNode.from() == bos) {
                        if (!d->ime_->model()->isNodeUnknown(latticeNode) &&
                            latticeNode.score() < min) {
                            min = latticeNode.score();
                        }
                        if (dup.count(latticeNode.word())) {
                            continue;
                        }
                        d->candidates_.push_back(
                            latticeNode.toSentenceResult());
                        dup.insert(latticeNode.word());
                    }
                }
            }
            for (auto &graphNode : graph.nodes(i)) {
                for (auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                    if (latticeNode.from() != bos &&
                        latticeNode.score() > min) {
                        auto fullWord = latticeNode.fullWord();
                        if (dup.count(fullWord)) {
                            continue;
                        }
                        d->candidates_.push_back(
                            latticeNode.toSentenceResult());
                    }
                }
            }
            std::sort(d->candidates_.begin() + beginSize, d->candidates_.end(),
                      std::greater<SentenceResult>());
        }
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

    if (d->selected.size()) {
        if (d->selected.back().second.back().offset_ == size()) {
            return true;
        }
    }

    return false;
}

std::string PinyinContext::selectedSentence() const {
    FCITX_D();
    std::stringstream ss;
    for (auto &s : d->selected) {
        for (auto &item : s.second) {
            ss << item.word_.word();
        }
    }
    return ss.str();
}

size_t PinyinContext::selectedLength() const {
    FCITX_D();
    if (d->selected.size()) {
        return d->selected.back().second.back().offset_;
    }
    return 0;
}

std::string PinyinContext::preedit() const { return preeditWithCursor().first; }

std::pair<std::string, size_t> PinyinContext::preeditWithCursor() const {
    FCITX_D();
    std::stringstream ss;
    auto sentence = selectedSentence();
    auto len = selectedLength();
    ss << sentence;
    auto c = cursor();
    size_t actualCursor = sentence.size();
    // should not happen
    if (c < len) {
        c = len;
    }

    auto resultSize = sentence.size();

    if (d->candidates_.size()) {
        bool first = true;
        for (auto &s : d->candidates_[0].sentence()) {
            for (auto iter = s->path().begin(),
                      end = std::prev(s->path().end());
                 iter < end; iter++) {
                if (!first) {
                    ss << " ";
                    resultSize += 1;
                } else {
                    first = false;
                }
                auto from = (*iter)->index(), to = (*std::next(iter))->index();
                if (c >= from + len && c < to + len) {
                    actualCursor = resultSize + c - from - len;
                }
                auto pinyin = d->segs_.segment(from, to);
                ss << pinyin;
                resultSize += pinyin.size();
            }
        }
    }
    if (c == size()) {
        actualCursor = resultSize;
    }
    return {ss.str(), actualCursor};
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
        for (auto &s : d->selected) {
            for (auto &item : s.second) {
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
    std::stringstream ss;
    std::string pinyin;
    if (d->selected.size() <= 1) {
        return false;
    }
    for (auto &s : d->selected) {
        bool first = true;
        for (auto &item : s.second) {
            if (!item.word_.word().empty()) {
                if (item.encodedPinyin_.size() != 3) {
                    return false;
                }
                if (first) {
                    first = false;
                    ss << item.word_.word();
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

    d->ime_->dict()->addWord(PinyinDictionary::UserDict, pinyin, ss.str());

    return true;
}

void PinyinContext::learnSentence() {}
}
