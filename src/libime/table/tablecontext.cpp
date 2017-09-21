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
#include "tablecontext.h"
#include "libime/core/decoder.h"
#include "libime/core/segmentgraph.h"
#include "libime/core/userlanguagemodel.h"
#include "libime/core/utils.h"
#include "tablebaseddictionary.h"
#include "tabledecoder.h"
#include "tableoptions.h"
#include "tablerule.h"
#include <boost/range/adaptor/filtered.hpp>
#include <fcitx-utils/log.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/utf8.h>

namespace libime {

namespace {

struct TableCandidateCompare {
    TableCandidateCompare(OrderPolicy policy, int noSortInputLength) : policy_(policy), noSortInputLength_(noSortInputLength) {}

    bool isNoSortInputLength(const SentenceResult &sentence) const {
        if (noSortInputLength_ < 0) {
            return false;
        }

        if (sentence.sentence().size() != 1) {
            return false;
        }
        auto node = static_cast<const TableLatticeNode*>(sentence.sentence()[0]);
        if (node->code().empty()) {
            return false;
        }
        return fcitx::utf8::length(node->code()) == static_cast<size_t>(noSortInputLength_);
    }

    size_t codeLength(const SentenceResult &sentence) const {
        auto node = static_cast<const TableLatticeNode*>(sentence.sentence()[0]);
        return fcitx::utf8::length(node->code());
    }

    size_t index(const SentenceResult &sentence) const {
        auto node = static_cast<const TableLatticeNode*>(sentence.sentence()[0]);
        return node->index();
    }

    bool operator()(const SentenceResult &lhs, const SentenceResult &rhs) const {
        bool lhsShort = isNoSortInputLength(lhs), rhsShort = isNoSortInputLength(rhs);
        // We want "true" to be put ahead.
        if (lhsShort != rhsShort) {
            return lhsShort > rhsShort;
        }
        if (lhsShort == rhsShort) {
            return index(lhs) < index(rhs);
        }

        switch (policy_) {
            case OrderPolicy::Fast:
                if (lhs.sentence().size() != rhs.sentence().size()) {
                    return lhs.sentence().size() < rhs.sentence().size();
                }
                return std::lexicographical_compare(lhs.sentence().begin(), lhs.sentence().end(),
                    rhs.sentence().begin(), rhs.sentence().end(),
                    [] (const LatticeNode *lnode, const LatticeNode *rnode) {
                        return static_cast<const TableLatticeNode *>(lnode)->index() < static_cast<const TableLatticeNode *>(rnode)->index();
                    }
                );

                break;
            case OrderPolicy::Freq:
                if (lhs.sentence().size() != rhs.sentence().size()) {
                    return lhs.sentence().size() < rhs.sentence().size();
                }
                return std::lexicographical_compare(lhs.sentence().begin(), lhs.sentence().end(),
                    rhs.sentence().begin(), rhs.sentence().end(),
                    [] (const LatticeNode *lnode, const LatticeNode *rnode) {
                        return static_cast<const TableLatticeNode *>(lnode)->score() > static_cast<const TableLatticeNode *>(rnode)->score();
                    }
                );
                break;
            case OrderPolicy::No:
            default:
                if (lhs.sentence().size() != rhs.sentence().size()) {
                    return lhs.sentence().size() < rhs.sentence().size();
                }
                return std::lexicographical_compare(lhs.sentence().begin(), lhs.sentence().end(),
                    rhs.sentence().begin(), rhs.sentence().end(),
                    [] (const LatticeNode *lnode, const LatticeNode *rnode) {
                        return static_cast<const TableLatticeNode *>(lnode)->index() < static_cast<const TableLatticeNode *>(rnode)->index();
                    }
                );
                break;
        }
    }

private:
    OrderPolicy policy_;
    int noSortInputLength_;
};

struct SelectedCode {
    SelectedCode(size_t s, WordNode word, std::string code)
        : offset_(s), word_(std::move(word)), code_(std::move(code)) {}
    size_t offset_;
    WordNode word_;
    std::string code_;
};

bool isPlaceHolder(const TableRuleEntry &entry) {
    return entry.isPlaceHolder();
}

bool checkRuleCanBeUsedAsAutoRule(const TableRule &rule) {
    if (rule.flag() != TableRuleFlag::LengthEqual) {
        return false;
    }

    auto range = rule.entries() | boost::adaptors::filtered(isPlaceHolder);
    auto iter = std::begin(range);
    auto end = std::end(range);
    int currentChar = 1;
    while (iter != end) {
        int currentIndex = 1;
        while (iter != end) {
            if (iter->character() == currentChar) {
                if (iter->flag() == TableRuleEntryFlag::FromFront &&
                    iter->encodingIndex() == currentIndex) {
                    currentIndex++;
                } else {
                    // reset to invalid.
                    currentIndex = 1;
                    break;
                }
            } else {
                break;
            }
            ++iter;
        }

        if (currentIndex == 1) {
            return false;
        }
        currentChar++;
    }
    return currentChar == rule.phraseLength() + 1;
}

SegmentGraph graphForCode(boost::string_view s,
                          const TableBasedDictionary &dict) {
    SegmentGraph graph(s.to_string());
    auto length = fcitx::utf8::length(s);
    graph.addNext(0, graph.size());
    if (dict.hasRule() && dict.tableOptions().autoRuleSet().size()) {
        const auto &ruleSet = dict.tableOptions().autoRuleSet();
        for (const auto &ruleName : ruleSet) {
            auto rule = dict.findRule(ruleName);
            if (!rule || length != rule->phraseLength() ||
                !checkRuleCanBeUsedAsAutoRule(*rule)) {
                continue;
            }

            std::vector<int> charSizes(rule->phraseLength());
            for (const auto &entry :
                 rule->entries() | boost::adaptors::filtered(isPlaceHolder)) {
                auto &charSize = charSizes[entry.character() - 1];
                charSize = std::max(charSize, entry.encodingIndex() - 1);
            }

            int lastIndex = 0;
            for (auto charSize : charSizes) {
                graph.addNext(lastIndex, lastIndex + charSize);
                lastIndex += charSize;
            }
        }
    }

    return graph;
}
}

class TableContextPrivate : public fcitx::QPtrHolder<TableContext> {
public:
    TableContextPrivate(TableContext *q, TableBasedDictionary &dict,
                        UserLanguageModel &model)
        : QPtrHolder(q), dict_(dict), model_(model), decoder_(&dict, &model) {}

    State currentState() {
        State state = model_.nullState();
        if (selected_.empty()) {
            return state;
        }
        State temp;
        for (auto &s : selected_) {
            for (auto &item : s) {
                if (item.word_.word().empty()) {
                    continue;
                }
                model_.score(state, item.word_, temp);
                state = std::move(temp);
            }
        }
        return state;
    }

    void resetMatchingState() {
        lattice_.clear();
        candidates_.clear();
        graph_ = SegmentGraph();
    }

    TableBasedDictionary &dict_;
    UserLanguageModel &model_;
    TableDecoder decoder_;
    Lattice lattice_;
    SegmentGraph graph_;
    std::vector<SentenceResult> candidates_;
    std::vector<std::vector<SelectedCode>> selected_;
};

TableContext::TableContext(TableBasedDictionary &dict, UserLanguageModel &model)
    : InputBuffer(fcitx::InputBufferOption::FixedCursor),
      d_ptr(std::make_unique<TableContextPrivate>(this, dict, model)) {}

TableContext::~TableContext() {}

const TableBasedDictionary &TableContext::dict() const {
    FCITX_D();
    return d->dict_;
}

bool TableContext::isValidInput(uint32_t c) const {
    FCITX_D();
    if (d->dict_.isInputCode(c)) {
        return true;
    }

    if (d->dict_.tableOptions().matchingKey() == c) {
        return true;
    }

    return false;
}

void TableContext::typeImpl(const char *s, size_t length) {
    boost::string_view view(s, length);
    auto utf8len = fcitx::utf8::lengthValidated(view);
    if (utf8len == fcitx::utf8::INVALID_LENGTH) {
        return;
    }

    auto range = fcitx::utf8::MakeUTF8CharRange(view);
    for (auto iter = range.begin(), end = range.end(); iter != end; iter++) {
        auto pair = iter.charRange();
        boost::string_view chr(&*pair.first,
                               std::distance(pair.first, pair.second));
        typeOneChar(chr);
    }
    update();
}

void TableContext::erase(size_t from, size_t to) {
    FCITX_D();
    if (from == 0 && to >= size()) {
        d->resetMatchingState();
        d->selected_.clear();
    }
    InputBuffer::erase(from, to);

    auto lastSeg = userInput().substr(selectedLength());
    d->graph_ = graphForCode(lastSeg, d->dict_);
    update();
}

void TableContext::select(size_t idx) {
    FCITX_D();
    assert(idx < d->candidates_.size());
    auto offset = selectedLength();
    d->selected_.emplace_back();

    auto &selection = d->selected_.back();
    for (auto &p : d->candidates_[idx].sentence()) {
        selection.emplace_back(
            offset + p->to()->index(),
            WordNode{p->word(), d->model_.index(p->word())},
            static_cast<const TableLatticeNode *>(p)->code());
    }

    update();
}

void TableContext::typeOneChar(boost::string_view chr) {
    FCITX_D();
    auto lastSeg = userInput().substr(selectedLength());
    auto lastSegLength = fcitx::utf8::length(lastSeg);
    // update userInput()
    InputBuffer::typeImpl(chr.data(), chr.size());

    auto &option = d->dict_.tableOptions();
    // Logic when append a new char:
    // Auto send disabled:
    // - keep append to buffer.
    // Auto send enabled:
    // - check no match auto select length.
    if (!option.autoSelect()) {
        lastSeg.append(chr.data(), chr.size());
        d->graph_ = graphForCode(lastSeg, d->dict_);
        return;
    }

    if (!lengthLessThanLimit(lastSegLength, d->dict_.maxLength()) ||
        (lastSegLength &&
         d->dict_.isEndKey(fcitx::utf8::getLastChar(lastSeg))) ||
        (d->dict_.tableOptions().noMatchAutoSelectLength() &&
         !lengthLessThanLimit(
             lastSegLength,
             d->dict_.tableOptions().noMatchAutoSelectLength()) &&
         !d->dict_.hasMatchingWords(lastSeg, chr))) {
        autoSelect();
        d->graph_ = graphForCode(chr, d->dict_);
    } else {
        lastSeg.append(chr.data(), chr.size());
        d->graph_ = graphForCode(lastSeg, d->dict_);
    }

    update();
}

void TableContext::autoSelect() {
    FCITX_D();
    if (selected()) {
        return;
    }

    if (d->candidates_.size()) {
        select(0);
    } else {
        if (d->dict_.tableOptions().commitRawInput()) {
            d->selected_.emplace_back();
            d->selected_.back().emplace_back(
                selectedLength() + d->graph_.data().size(),
                WordNode{d->graph_.data(), d->model_.unknown()},
                d->graph_.data());
        } else {
            auto lastSegLength = fcitx::utf8::length(d->graph_.data());
            erase(size() - lastSegLength, size());
        }
    }

    update();
}

void TableContext::update() {
    FCITX_D();
    if (!size()) {
        return;
    }

    if (selected()) {
        d->resetMatchingState();
        return;
    }

    d->lattice_.clear();
    State state = d->currentState();
    d->decoder_.decode(d->lattice_, d->graph_, 1, state);

    d->candidates_.clear();

    auto &graph = d->graph_;
    auto bos = &graph.start();
    for (size_t i = graph.size(); i > 0; i--) {
        for (auto &graphNode : graph.nodes(i)) {
            for (auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                if (latticeNode.from() == bos) {
                    d->candidates_.push_back(latticeNode.toSentenceResult());
                }
            }
        }
    }
    int lastSegLength = fcitx::utf8::length(d->graph_.data());
    int noSortLength = lastSegLength < d->dict_.tableOptions().noSortInputLength() ? lastSegLength : d->dict_.tableOptions().noSortInputLength();
    std::sort(d->candidates_.begin(), d->candidates_.end(),
              TableCandidateCompare(d->dict_.tableOptions().orderPolicy(), noSortLength));
    // Run auto select.
    if (d->dict_.tableOptions().autoSelect()) {
        if (d->candidates_.size() == 1 &&
            !lengthLessThanLimit(lastSegLength,
                                 d->dict_.tableOptions().autoSelectLength())) {
            autoSelect();
        }
    }
}

const std::vector<SentenceResult> &TableContext::candidates() const {
    FCITX_D();
    return d->candidates_;
}

size_t TableContext::selectedLength() const {
    FCITX_D();
    if (d->selected_.size()) {
        return d->selected_.back().back().offset_;
    }
    return 0;
}

std::string TableContext::selectedSentence() const {
    FCITX_D();
    std::string ss;
    for (auto &s : d->selected_) {
        for (auto &item : s) {
            ss += item.word_.word();
        }
    }
    return ss;
}

std::string TableContext::preedit() const {
    std::string ss = selectedSentence();
    ss += userInput().substr(selectedLength());
    return ss;
}

bool TableContext::selected() const {
    FCITX_D();
    if (userInput().empty() || d->selected_.empty()) {
        return false;
    }
    return d->selected_.back().back().offset_ == userInput().size();
}

void TableContext::learn() {}

std::vector<std::string> TableContext::candidateHint(size_t idx,
                                                     bool custom) const {
    FCITX_D();
    std::vector<std::string> codes;
    for (auto &p : d->candidates_[idx].sentence()) {
        if (!p->word().empty()) {
            const auto &code = static_cast<const TableLatticeNode *>(p)->code();
            if (custom) {
                codes.push_back(d->dict_.hint(code));
            } else {
                codes.push_back(code);
            }
        }
    }
    return codes;
}
}
