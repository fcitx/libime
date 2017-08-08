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
#include <fcitx-utils/log.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/utf8.h>

namespace libime {

struct SelectedCode {
    SelectedCode(size_t s, WordNode word, std::string code)
        : offset_(s), word_(std::move(word)),
          code_(std::move(code)) {}
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
    std::vector<std::pair<bool, std::vector<SelectedCode>>> selected_;
};

TableContext::TableContext(TableBasedDictionary &dict, UserLanguageModel &model)
    : InputBuffer(true),
      d_ptr(std::make_unique<TableContextPrivate>(dict, model)) {}

TableContext::~TableContext() {}

const TableBasedDictionary &TableContext::dict() const {
    FCITX_D();
    return d->dict_;
}

bool TableContext::cancelTill(size_t pos) { return false; }

bool TableContext::isValidInput(char c) const {
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

bool TableContext::isEndKey(char c) const {
    FCITX_D();
    return d->dict_.tableOptions().endKey().find(c) != std::string::npos;
}

void TableContext::typeImpl(const char *s, size_t length) {
    cancelTill(cursor());
    boost::string_view view(s, length);
    auto utf8len = fcitx::utf8::lengthValidated(view.begin(), view.end());
    if (utf8len != fcitx::utf8::INVALID_LENGTH && utf8len > 0) {
        for (auto iter = view.begin(), next = fcitx::utf8::nextChar(iter);
             next <= view.end();
             iter = next, next = fcitx::utf8::nextChar(next)) {
            typeOneChar(iter, next - iter);
            update();
        }
    }
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

    if (to != size()) {
        return;
    }

    // check if erase everything
    if (from == 0 && to >= size()) {
        FCITX_D();
        d->candidates_.clear();
        d->selected_.clear();
        d->lattice_.clear();
        d->graph_ = SegmentGraph();
        // FIXME
    } else {
        cancelTill(from);
    }
    InputBuffer::erase(from, to);
    d->graph_.removeSuffixFrom(from);

    if (size()) {
        update();
    }
}

void TableContext::update() {
    FCITX_D();

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
        float min = 0;
        float max = -std::numeric_limits<float>::max();
        //
        auto adjust = static_cast<float>(graph.size() - i) *
                      d->model_.unknownPenalty() / 10;
        for (auto &graphNode : graph.nodes(i)) {
            for (auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                if (latticeNode.from() == bos) {
                    if (!d->model_.isNodeUnknown(latticeNode)) {
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
            for (auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                if (latticeNode.from() != bos) {
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

boost::string_view lastSegment(const SegmentGraph &graph) {
    if (!graph.size()) {
        return {};
    } else {
        assert(graph.end().prevSize());
        return graph.segment(graph.end().prevs().front(), graph.end());
    }
}

void TableContext::typeOneChar(const char *s, size_t length) {
    FCITX_D();
    InputBuffer::typeImpl(s, length);
    auto &option = d->dict_.tableOptions();
    if (option.pinyinKey() &&
        fcitx::stringutils::startsWith(userInput(), option.pinyinKey())) {
        // TODO: do pinyin stuff.
        return;
    }

    auto lastSeg = lastSegment(d->graph_);
    FCITX_LOG(Info) << lastSeg;
    if (lastSeg.size() >= d->dict_.maxLength()
        || (lastSeg.size() > 0 && isEndKey(lastSeg.back()))
        || (d->dict_.tableOptions().noMatchAutoCommitLength() && lastSeg.size() >= d->dict_.tableOptions().noMatchAutoCommitLength() && !d->dict_.hasMatchingWords(lastSeg, {s, length}))) {
        d->graph_.appendNewSegment({s, length});
    } else {
        d->graph_.appendToLastSegment({s, length});
    }
}

const std::vector<SentenceResult> &TableContext::candidates() const {
    FCITX_D();
    return d->candidates_;
}
}
