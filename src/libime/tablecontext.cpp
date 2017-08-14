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
#include "tablecontext.h"
#include "decoder.h"
#include "segmentgraph.h"
#include "tablebaseddictionary.h"
#include "tableoptions.h"
#include "userlanguagemodel.h"
#include "utils.h"
#include <fcitx-utils/log.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/utf8.h>

namespace libime {

struct SelectedCode {
    SelectedCode(size_t s, WordNode word, std::string code)
        : offset_(s), word_(std::move(word)), code_(std::move(code)) {}
    size_t offset_;
    WordNode word_;
    std::string code_;
};

class TableContextPrivate {
public:
    TableContextPrivate(TableBasedDictionary &dict, UserLanguageModel &model)
        : dict_(dict), model_(model), decoder_(&dict, &model) {}

    TableBasedDictionary &dict_;
    UserLanguageModel &model_;
    Decoder decoder_;
    Lattice lattice_;
    SegmentGraph graph_;
    std::vector<SentenceResult> candidates_;
    std::vector<SelectedCode> selected_;
};

TableContext::TableContext(TableBasedDictionary &dict, UserLanguageModel &model)
    : InputBuffer(true),
      d_ptr(std::make_unique<TableContextPrivate>(dict, model)) {}

TableContext::~TableContext() {}

const TableBasedDictionary &TableContext::dict() const {
    FCITX_D();
    return d->dict_;
}

bool TableContext::cancelTill(size_t pos) {
    bool cancelled = false;
    while (selectedLength() > pos) {
        cancel();
        cancelled = true;
    }
    return cancelled;
}

void TableContext::cancel() {
    FCITX_D();

    if (d->selected_.size()) {
        d->selected_.pop_back();
    }
    update();
}

bool TableContext::isValidInput(uint32_t c) const {
    FCITX_D();
    if (d->dict_.isInputCode(c)) {
        return true;
    }

    if (d->dict_.tableOptions().matchingKey() == c) {
        return true;
    }

    if (d->dict_.tableOptions().pinyinKey() == c) {
        return true;
    }
    if (d->dict_.tableOptions().pinyinKey()) {
        const boost::string_view validPinyin("abcdefghijklmnopqrstuvwxyz");
        if (validPinyin.find(c) != boost::string_view::npos) {
            return true;
        }
    }

    return false;
}

bool TableContext::isEndKey(uint32_t c) const {
    FCITX_D();
    return !!d->dict_.tableOptions().endKey().count(c);
}

void TableContext::typeImpl(const char *s, size_t length) {
    auto oldCursor = cursor();
    cancelTill(oldCursor);
    boost::string_view view(s, length);
    auto utf8len = fcitx::utf8::lengthValidated(view);
    if (utf8len == fcitx::utf8::INVALID_LENGTH) {
        return;
    }
    InputBuffer::typeImpl(s, length);
    reparseFrom(oldCursor);
    update();
}

void TableContext::setCursor(size_t pos) {
    auto cancelled = cancelTill(pos);
    InputBuffer::setCursor(pos);
    if (cancelled) {
        update();
    }
}

void TableContext::erase(size_t from, size_t to) {
    FCITX_D();
    if (from == to) {
        return;
    }

    if (to > size()) {
        return;
    }

    // check if erase everything
    if (from == 0 && to >= size()) {
        FCITX_D();
        d->candidates_.clear();
        d->selected_.clear();
        d->lattice_.clear();
        d->graph_ = SegmentGraph();
    } else {
        cancelTill(from);
    }
    InputBuffer::erase(from, to);
    reparseFrom(from);
    update();
}

boost::string_view firstSegment(const SegmentGraph &graph) {
    if (!graph.size()) {
        return {};
    } else {
        assert(graph.start().nextSize());
        return graph.segment(graph.start(), graph.start().nexts().front());
    }
}

boost::string_view lastSegment(const SegmentGraph &graph) {
    if (!graph.size()) {
        return {};
    } else {
        assert(graph.end().prevSize());
        return graph.segment(graph.end().prevs().front(), graph.end());
    }
}

void TableContext::reparseFrom(size_t from) {
    FCITX_D();
    auto &option = d->dict_.tableOptions();
    if (option.pinyinKey() &&
        fcitx::stringutils::startsWith(userInput(), option.pinyinKey())) {
        // TODO: do pinyin stuff.
        return;
    }

    // Maintain the equality of graph_ and userInput().
    size_t byteStart = 0;
    if (from == 0) {
        byteStart = 0;
    } else if (from == size()) {
        byteStart = userInput().size();
    } else {
        byteStart = rangeAt(from).first;
    }

    d->graph_.removeSuffixFrom(byteStart);
    boost::string_view view = userInput();
    assert(d->graph_.data() == view.substr(0, byteStart));
    view = view.substr(byteStart);
    auto range = fcitx::utf8::MakeUTF8CharRange(view);
    for (auto iter = range.begin(), end = range.end(); iter != end; iter++) {
        auto lastSeg = lastSegment(d->graph_);
        auto charRange = iter.charRange();
        boost::string_view currentChar(
            &*charRange.first,
            std::distance(charRange.first, charRange.second));
        auto lastSegLength = fcitx::utf8::length(lastSeg);
        if (!lengthLessThanLimit(lastSegLength, d->dict_.maxLength()) ||
            (lastSegLength && isEndKey(fcitx::utf8::getLastChar(lastSeg))) ||
            (d->dict_.tableOptions().noMatchAutoSelectLength() &&
             !lengthLessThanLimit(
                 lastSegLength,
                 d->dict_.tableOptions().noMatchAutoSelectLength()) &&
             !d->dict_.hasMatchingWords(lastSeg, currentChar))) {
            d->graph_.appendNewSegment(currentChar);
        } else {
            d->graph_.appendToLastSegment(currentChar);
        }
    }
}

void TableContext::update() {
    FCITX_D();
    if (!size()) {
        return;
    }

    // Run auto select.
    if (d->dict_.tableOptions().autoSelect()) {
    }

    d->lattice_.clear();
    d->decoder_.decode(d->lattice_, d->graph_, 1, d->model_.nullState());

    d->candidates_.clear();
    std::unordered_set<std::string> dup;
    for (size_t i = 0, e = d->lattice_.sentenceSize(); i < e; i++) {
        d->candidates_.push_back(d->lattice_.sentence(i));
        dup.insert(d->candidates_.back().toString());
    }

    auto &graph = d->graph_;
    auto bos = &graph.start();
    auto beginSize = d->candidates_.size();
    for (size_t i = graph.size(); i > 0; i--) {
        for (auto &graphNode : graph.nodes(i)) {
            for (auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                if (latticeNode.from() == bos) {
                    if (dup.count(latticeNode.word())) {
                        continue;
                    }
                    d->candidates_.push_back(latticeNode.toSentenceResult());
                    dup.insert(latticeNode.word());
                }
            }
        }
    }
    std::sort(d->candidates_.begin() + beginSize, d->candidates_.end(),
              std::greater<SentenceResult>());
}

const std::vector<SentenceResult> &TableContext::candidates() const {
    FCITX_D();
    return d->candidates_;
}

size_t TableContext::selectedLength() const {
    FCITX_D();
    if (d->selected_.size()) {
        return d->selected_.back().offset_;
    }
    return 0;
}
}
