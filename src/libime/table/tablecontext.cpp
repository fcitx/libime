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
#include "libime/core/historybigram.h"
#include "libime/core/segmentgraph.h"
#include "libime/core/userlanguagemodel.h"
#include "libime/core/utils.h"
#include "tablebaseddictionary.h"
#include "tabledecoder.h"
#include "tableoptions.h"
#include "tablerule.h"
#include <fcitx-utils/log.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/utf8.h>

namespace libime {

namespace {

struct TableCandidateCompare {
    TableCandidateCompare(OrderPolicy policy, int noSortInputLength)
        : policy_(policy), noSortInputLength_(noSortInputLength) {}

    bool isNoSortInputLength(const SentenceResult &sentence) const {
        if (noSortInputLength_ < 0) {
            return false;
        }

        if (sentence.sentence().size() != 1) {
            return false;
        }
        auto node =
            static_cast<const TableLatticeNode *>(sentence.sentence()[0]);
        if (node->code().empty()) {
            return false;
        }
        return fcitx::utf8::length(node->code()) ==
               static_cast<size_t>(noSortInputLength_);
    }

    static size_t codeLength(const SentenceResult &sentence) {
        auto node =
            static_cast<const TableLatticeNode *>(sentence.sentence()[0]);
        return fcitx::utf8::length(node->code());
    }

    static size_t index(const SentenceResult &sentence) {
        auto node =
            static_cast<const TableLatticeNode *>(sentence.sentence()[0]);
        return node->index();
    }

    bool operator()(const SentenceResult &lhs,
                    const SentenceResult &rhs) const {
        if (lhs.size() != rhs.size()) {
            return lhs.size() < rhs.size();
        }
        // single word.
        if (lhs.size() == 1) {
            bool lShort =
                static_cast<int>(codeLength(lhs)) == noSortInputLength_;
            bool rShort =
                static_cast<int>(codeLength(rhs)) == noSortInputLength_;
            if (lShort != rShort) {
                return lShort > rShort;
            }

            auto policy = lShort ? OrderPolicy::No : policy_;

            switch (policy) {
            case OrderPolicy::No:
            case OrderPolicy::Fast:
                return index(lhs) < index(rhs);
            case OrderPolicy::Freq:
                return lhs.score() > rhs.score();
            }
            return false;
        }

        return lhs.score() > rhs.score();
    }

private:
    OrderPolicy policy_;
    int noSortInputLength_;
};

struct SelectedCode {
    SelectedCode(size_t offset, WordNode word, std::string code,
                 bool commit = true)
        : offset_(offset), word_(std::move(word)), code_(std::move(code)),
          commit_(commit) {}
    size_t offset_;
    WordNode word_;
    std::string code_;
    bool commit_;
};

bool shouldReplaceCandidate(const SentenceResult &oldSentence,
                            const SentenceResult &newSentence) {
    if (newSentence.sentence().size() != oldSentence.sentence().size()) {
        return newSentence.sentence().size() < oldSentence.sentence().size();
    }
    // sentence size are equal, prefer shorter code.
    if (newSentence.sentence().size() == 1) {
        return TableCandidateCompare::codeLength(newSentence) <
               TableCandidateCompare::codeLength(oldSentence);
    }
    return false;
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

    size_t selectedLength() const {
        if (selected_.size()) {
            return selected_.back().back().offset_;
        }
        return 0;
    }

    void cancel() {
        if (selected_.size()) {
            selected_.pop_back();
        }
    }

    bool cancelTill(size_t pos) {
        bool cancelled = false;
        while (selectedLength() > pos) {
            cancel();
            cancelled = true;
        }
        return cancelled;
    }

    bool learnWord(const std::vector<SelectedCode> &selection) {
        std::string word;
        for (const auto &selected : selection) {
            if (!selected.commit_) {
                return true;
            }
            word += selected.word_.word();
        }
        return dict_.insert(word, PhraseFlag::User);
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
        InputBuffer::erase(from, to);
    } else {
        d->cancelTill(from);
        InputBuffer::erase(from, to);

        auto lastSeg = userInput().substr(selectedLength());
        d->graph_ = graphForCode(lastSeg, d->dict_);
    }
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
        if (currentCode().empty()) {
            return;
        }
        // Need to calculate this first, otherwise we're breaking the contract
        // of selected_ (contains no zero-length vec).
        auto offset = selectedLength();
        d->selected_.emplace_back();
        d->selected_.back().emplace_back(
            offset + d->graph_.data().size(),
            WordNode{d->graph_.data(), d->model_.unknown()}, d->graph_.data(),
            d->dict_.tableOptions().commitRawInput());
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

    d->candidates_.clear();

    int lastSegLength = fcitx::utf8::length(d->graph_.data());
    if (d->decoder_.decode(d->lattice_, d->graph_, 5, state)) {
        std::unordered_map<std::string, size_t> dup;

        auto insertCandidate = [d, &dup](SentenceResult sentence) {
            auto sentenceString = sentence.toString();
            auto iter = dup.find(sentenceString);
            if (iter != dup.end()) {
                auto idx = iter->second;
                if (shouldReplaceCandidate(d->candidates_[idx], sentence)) {
                    d->candidates_[idx] = std::move(sentence);
                }
            } else {
                d->candidates_.push_back(std::move(sentence));
                dup[sentenceString] = d->candidates_.size() - 1;
            }
        };

        auto &graph = d->graph_;
        auto bos = &graph.start(), eos = &graph.end();
        for (auto &latticeNode : d->lattice_.nodes(eos)) {
            if (latticeNode.from() == bos && latticeNode.to() == eos) {
                insertCandidate(latticeNode.toSentenceResult());
            }
        }

        float min = 0;
        for (const auto &cand : d->candidates_) {
            if (min > cand.score()) {
                min = cand.score();
            }
        }

        // FIXME: add an option.
        const float minDistance = 1.0f;
        for (size_t i = 0, e = d->lattice_.sentenceSize(); i < e; i++) {
            const auto &sentence = d->lattice_.sentence(i);
            auto score = sentence.score();
            if (sentence.sentence().size() >= 1) {
                score = sentence.sentence().back()->score();
            }
            if (min - score < minDistance) {
                insertCandidate(sentence);
            }
        }
        int noSortLength =
            lastSegLength < d->dict_.tableOptions().noSortInputLength()
                ? lastSegLength
                : d->dict_.tableOptions().noSortInputLength();
        std::sort(d->candidates_.begin(), d->candidates_.end(),
                  TableCandidateCompare(d->dict_.tableOptions().orderPolicy(),
                                        noSortLength));
    }
    // Run auto select.
    if (d->dict_.tableOptions().autoSelect()) {
        if (d->candidates_.size() <= 1 &&
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
    return d->selectedLength();
}

std::string TableContext::selectedSentence() const {
    FCITX_D();
    std::string ss;
    for (auto &s : d->selected_) {
        for (auto &item : s) {
            if (item.commit_) {
                ss += item.word_.word();
            }
        }
    }
    return ss;
}

const std::string &TableContext::currentCode() const {
    FCITX_D();
    return d->graph_.data();
}

bool TableContext::selected() const {
    FCITX_D();
    if (userInput().empty() || d->selected_.empty()) {
        return false;
    }
    return d->selected_.back().back().offset_ == userInput().size();
}

size_t TableContext::selectedSize() const {
    FCITX_D();
    return d->selected_.size();
}

std::tuple<std::string, bool> TableContext::selectedSegment(size_t idx) const {
    FCITX_D();
    std::string result;
    bool commit = true;
    for (auto &item : d->selected_[idx]) {
        if (!item.commit_) {
            commit = false;
        }
        result += item.word_.word();
    }
    return {std::move(result), commit};
}

std::string TableContext::preedit() const {
    std::string result;
    for (size_t i = 0, e = selectedSize(); i < e; i++) {
        auto seg = selectedSegment(i);
        if (std::get<bool>(seg)) {
            result += std::get<std::string>(seg);
        } else {
            result += "(";
            result += std::get<std::string>(seg);
            result += ")";
        }
    }
    result += currentCode();
    return result;
}

void TableContext::learn() {
    FCITX_D();
    if (!selected()) {
        return;
    }

    for (auto &s : d->selected_) {
        if (s.size() > 1) {
            if (!d->learnWord(s)) {
                return;
            }
        }
    }
    std::vector<std::string> newSentence;
    for (auto &s : d->selected_) {
        std::string word;
        for (auto &item : s) {
            if (!item.commit_) {
                word.clear();
                break;
            }
            word += item.word_.word();
        }
        if (!word.empty()) {
            newSentence.emplace_back(std::move(word));
        }
    }
    if (newSentence.size()) {
        d->model_.history().add(newSentence);
    }
}

std::string TableContext::candidateHint(size_t idx, bool custom) const {
    FCITX_D();
    if (d->candidates_[idx].sentence().size() == 1) {
        auto p = d->candidates_[idx].sentence()[0];
        if (!p->word().empty()) {
            boost::string_view code =
                static_cast<const TableLatticeNode *>(p)->code();
            code.remove_prefix(currentCode().size());
            if (custom) {
                return d->dict_.hint(code);
            } else {
                return code.to_string();
            }
        }
    }
    return {};
}
}
