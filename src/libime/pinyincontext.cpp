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
#include "pinyindecoder.h"
#include "pinyinencoder.h"
#include "pinyinime.h"
#include <algorithm>
#include <boost/range/adaptor/transformed.hpp>
#include <iostream>

namespace libime {

class PinyinContextPrivate {
public:
    PinyinContextPrivate(PinyinIME *ime) : ime_(ime) {}

    std::vector<std::vector<std::pair<size_t, std::string>>> selected;

    PinyinIME *ime_;
    SegmentGraph segs_;
    Lattice lattice_;
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
    FCITX_D();
    cancelTill(from);
    InputBuffer::erase(from, to);
    update();
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
    for (auto &p : d->candidates_[idx].sentence()) {
        selection.emplace_back(offset + p.first->index(), p.second.to_string());
    }
    // add some special code for handling separator at the end
    auto remain = boost::string_view(userInput()).substr(selectedLength());
    if (!remain.empty()) {
        if (std::all_of(remain.begin(), remain.end(),
                        [](char c) { return c == '\''; })) {
            selection.emplace_back(size(), "");
        }
    }

    update();
}

void PinyinContext::cancelTill(size_t pos) {
    while (selectedLength() > pos) {
        cancel();
    }
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
    size_t start = 0;
    if (d->selected.size()) {
        start = d->selected.back().back().first;
    }
    d->segs_ = PinyinEncoder::parseUserPinyin(
        boost::string_view(userInput()).substr(start), d->ime_->fuzzyFlags());
    auto &graph = d->segs_;

    d->lattice_ = d->ime_->decoder()->decode(
        d->segs_, d->ime_->nbest(), std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max());

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
                    if (latticeNode.idx() != d->ime_->model()->unknown() &&
                        latticeNode.score() < min) {
                        min = latticeNode.score();
                    }
                    if (dup.count(latticeNode.word())) {
                        continue;
                    }
                    d->candidates_.push_back(latticeNode.toSentenceResult());
                    dup.insert(latticeNode.word());
                }
            }
        }
        for (auto &graphNode : graph.nodes(i)) {
            for (auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                if (latticeNode.from() != bos && latticeNode.score() > min) {
                    auto fullWord = latticeNode.fullWord();
                    if (dup.count(fullWord)) {
                        continue;
                    }
                    d->candidates_.push_back(latticeNode.toSentenceResult());
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

    if (d->selected.size()) {
        if (d->selected.back().back().first == size()) {
            return true;
        }
    }

    return false;
}

std::string PinyinContext::selectedSentence() const {
    FCITX_D();
    std::stringstream ss;
    for (auto &s : d->selected) {
        for (auto &item : s) {
            ss << item.second;
        }
    }
    return ss.str();
}

size_t PinyinContext::selectedLength() const {
    FCITX_D();
    if (d->selected.size()) {
        return d->selected.back().back().first;
    }
    return 0;
}

std::string PinyinContext::preedit() const {
    FCITX_D();
    std::stringstream ss;
    ss << selectedSentence();

    if (d->candidates_.size()) {
        size_t start = 0;
        bool first = true;
        for (auto &s : d->candidates_[0].sentence()) {
            if (!first) {
                ss << " ";
            } else {
                first = false;
            }
            ss << d->segs_.segment(start, s.first->index());
            start = s.first->index();
        }
    }
    return ss.str();
}
}
