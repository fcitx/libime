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
#include "constants.h"
#include "libime/core/decoder.h"
#include "libime/core/historybigram.h"
#include "libime/core/segmentgraph.h"
#include "libime/core/userlanguagemodel.h"
#include "libime/core/utils.h"
#include "log.h"
#include "tablebaseddictionary.h"
#include "tabledecoder.h"
#include "tableoptions.h"
#include "tablerule.h"
#include <boost/ptr_container/ptr_vector.hpp>
#include <chrono>
#include <fcitx-utils/log.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/utf8.h>

namespace libime {

namespace {

struct TableCandidateCompare {
    TableCandidateCompare(OrderPolicy policy, int noSortInputLength,
                          size_t codeLength)
        : policy_(policy), noSortInputLength_(noSortInputLength),
          codeLength_(codeLength) {}

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

    static PhraseFlag flag(const SentenceResult &sentence) {
        auto node =
            static_cast<const TableLatticeNode *>(sentence.sentence()[0]);
        return node->flag();
    }

    // Larger index should be put ahead.
    static int64_t index(const SentenceResult &sentence) {
        auto node =
            static_cast<const TableLatticeNode *>(sentence.sentence()[0]);
        if (node->flag() == PhraseFlag::User) {
            return node->index();
        } else {
            return -static_cast<int64_t>(node->index());
        }
    }

    static bool isPinyin(const SentenceResult &sentence) {
        return sentence.size() == 1 && flag(sentence) == PhraseFlag::Pinyin;
    }

    static bool isAuto(const SentenceResult &sentence) {
        return sentence.size() != 1 || flag(sentence) == PhraseFlag::Auto;
    }

    bool isWithinNoSortLimit(const SentenceResult &sentence) const {
        return static_cast<int>(codeLength(sentence)) == noSortInputLength_ &&
               !isPinyin(sentence);
    }

    bool operator()(const SentenceResult &lhs,
                    const SentenceResult &rhs) const {
        bool lIsAuto = isAuto(lhs);
        bool rIsAuto = isAuto(rhs);
        if (lIsAuto != rIsAuto) {
            return lIsAuto < rIsAuto;
        }
        // non-auto word
        if (!lIsAuto) {
            bool lIsPinyin = isPinyin(lhs);
            bool rIsPinyin = isPinyin(rhs);
            bool lShort = isWithinNoSortLimit(lhs);
            bool rShort = isWithinNoSortLimit(rhs);
            if (lShort != rShort) {
                return lShort > rShort;
            }

            auto policy = lShort ? OrderPolicy::No : policy_;
            constexpr float pinyinPenalty = -0.5;
            constexpr float exactMatchAward = 1;

            switch (policy) {
            case OrderPolicy::No:
            case OrderPolicy::Fast:
                return index(lhs) > index(rhs);
            case OrderPolicy::Freq: {
                bool lExact = codeLength(lhs) == codeLength_;
                bool rExact = codeLength(rhs) == codeLength_;
                return lhs.score() + (lIsPinyin ? pinyinPenalty : 0) +
                           (lExact ? exactMatchAward : 0) >
                       rhs.score() + (rIsPinyin ? pinyinPenalty : 0) +
                           (rExact ? exactMatchAward : 0);
            }
            }
            return false;
        }

        return lhs.score() > rhs.score();
    }

private:
    OrderPolicy policy_;
    int noSortInputLength_;
    size_t codeLength_;
};

struct SelectedCode {
    SelectedCode(size_t offset, WordNode word, std::string code,
                 PhraseFlag flag, bool commit = true)
        : offset_(offset), word_(std::move(word)), code_(std::move(code)),
          flag_(flag), commit_(commit) {}
    size_t offset_;
    WordNode word_;
    std::string code_;
    PhraseFlag flag_;
    bool commit_;
};

bool shouldReplaceCandidate(const SentenceResult &oldSentence,
                            const SentenceResult &newSentence,
                            OrderPolicy policy) {
    if (newSentence.sentence().size() != oldSentence.sentence().size()) {
        return newSentence.sentence().size() < oldSentence.sentence().size();
    }
    // sentence size are equal, prefer shorter code.
    if (newSentence.sentence().size() == 1) {
        auto oldCode = TableCandidateCompare::codeLength(newSentence);
        auto newCode = TableCandidateCompare::codeLength(oldSentence);

        if (oldCode != newCode) {
            return oldCode < newCode;
        }

        auto newNode =
            static_cast<const TableLatticeNode *>(newSentence.sentence()[0]);
        if (policy == OrderPolicy::No && newNode->flag() != PhraseFlag::User) {
            return true;
        }
        if (policy != OrderPolicy::No && newNode->flag() == PhraseFlag::User) {
            return true;
        }
    }

    return false;
}
} // namespace

class TableContextPrivate : public fcitx::QPtrHolder<TableContext> {
public:
    TableContextPrivate(TableContext *q, TableBasedDictionary &dict,
                        UserLanguageModel &model)
        : QPtrHolder(q), dict_(dict), model_(model), decoder_(&dict, &model) {
        // Maybe use a better heuristics?
        candidates_.reserve(2048);
    }

    // sort should already happened at this point.
    bool canDoAutoSelect() const {
        if (candidates_.size() == 0) {
            return false;
        }
        return !TableCandidateCompare::isAuto(candidates_[0]) &&
               !TableCandidateCompare::isPinyin(candidates_[0]);
    };

    // sort should already happened at this point.
    bool hasOnlyOneAutoselectChoice() const {
        if (!canDoAutoSelect()) {
            return false;
        }
        if (candidates_.size() == 1) {
            return true;
        }

        for (size_t idx = 1, e = candidates_.size(); idx < e; idx++) {
            // Optimization: auto is always at the end, once we see it, we
            // should be know there is no other choice.
            if (TableCandidateCompare::isAuto(candidates_[idx])) {
                return true;
            }
            // Do not auto select on pinyin.
            if (TableCandidateCompare::isPinyin(candidates_[idx])) {
                continue;
            }
            return false;
        }
        return true;
    };

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
        if (selection.size() == 1) {
            auto &select = selection[0];
            if (select.flag_ == PhraseFlag::None ||
                select.flag_ == PhraseFlag::User) {
                dict_.insert(select.code_, select.word_.word(),
                             PhraseFlag::User);
            } else if (select.flag_ == PhraseFlag::Auto) {
                // Remove from auto.
                dict_.removeWord(select.code_, select.word_.word());
                dict_.insert(select.code_, select.word_.word(),
                             PhraseFlag::User);
            }

            return true;
        }
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

TableBasedDictionary &TableContext::mutableDict() {
    FCITX_D();
    return d->dict_;
}

const UserLanguageModel &TableContext::model() const {
    FCITX_D();
    return d->model_;
}

UserLanguageModel &TableContext::mutableModel() {
    FCITX_D();
    return d->model_;
}

bool TableContext::isValidInput(uint32_t c) const {
    FCITX_D();
    auto matchingKey = d->dict_.tableOptions().matchingKey();
    return (d->dict_.isInputCode(c) || (matchingKey && matchingKey == c) ||
            (d->dict_.hasPinyin() && (c <= 'z' && c >= 'a')));
}

bool TableContext::typeImpl(const char *s, size_t length) {
    std::string_view view(s, length);
    auto utf8len = fcitx::utf8::lengthValidated(view);
    if (utf8len == fcitx::utf8::INVALID_LENGTH) {
        return false;
    }

    bool changed = false;
    auto range = fcitx::utf8::MakeUTF8CharRange(view);
    for (auto iter = range.begin(), end = range.end(); iter != end; iter++) {
        auto pair = iter.charRange();
        std::string_view chr(&*pair.first,
                             std::distance(pair.first, pair.second));
        if (!typeOneChar(chr)) {
            break;
        }
        changed = true;
    }
    return changed;
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
        auto node = static_cast<const TableLatticeNode *>(p);
        selection.emplace_back(offset + p->to()->index(),
                               WordNode{p->word(), d->model_.index(p->word())},
                               node->code(), node->flag());
    }

    update();
}

bool TableContext::typeOneChar(std::string_view chr) {
    FCITX_D();
    auto lastSeg = userInput().substr(selectedLength());
    auto lastSegLength = fcitx::utf8::length(lastSeg);
    // update userInput()
    if (!InputBuffer::typeImpl(chr.data(), chr.size())) {
        return false;
    }

    auto &option = d->dict_.tableOptions();
    // Logic when append a new char:
    // Auto send disabled:
    // - keep append to buffer.
    // Auto send enabled:
    // - check no match auto select length.
    bool doAutoSelect = option.autoSelect();
    if (doAutoSelect) {
        // No pinyin, because pinyin has no limit on length.
        // Also, check if it exceeds the code length.
        doAutoSelect =
            (!d->dict_.hasPinyin() &&
             !lengthLessThanLimit(lastSegLength, d->dict_.maxLength()));
        // Check if it
        doAutoSelect = doAutoSelect ||
                       (lastSegLength &&
                        d->dict_.isEndKey(fcitx::utf8::getLastChar(lastSeg)));
        // Check no match auto select.
        // It means "last segement + chr" has no match, so
        // we just select lastSeg instead.
        doAutoSelect =
            doAutoSelect ||
            (d->dict_.tableOptions().noMatchAutoSelectLength() &&
             !lengthLessThanLimit(
                 lastSegLength,
                 d->dict_.tableOptions().noMatchAutoSelectLength()) &&
             !d->dict_.hasMatchingWords(lastSeg, chr));
    }

    if (doAutoSelect) {
        autoSelect();
        d->graph_ = graphForCode(chr, d->dict_);
    } else {
        lastSeg.append(chr.data(), chr.size());
        d->graph_ = graphForCode(lastSeg, d->dict_);
    }

    update();
    return true;
}

void TableContext::autoSelect() {
    FCITX_D();
    if (selected()) {
        return;
    }

    if (d->canDoAutoSelect()) {
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
            PhraseFlag::Invalid, d->dict_.tableOptions().commitRawInput());
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

    auto t0 = std::chrono::high_resolution_clock::now();
    d->candidates_.clear();

    constexpr float max = std::numeric_limits<float>::max();
    constexpr float min = -std::numeric_limits<float>::max();
    constexpr int beamSize = 20;
    constexpr int frameSize = 10;
    auto lastSegLength = fcitx::utf8::length(d->graph_.data());
    int nbest = 1;
    if (lastSegLength == d->dict_.maxLength() &&
        !d->dict_.tableOptions().autoRuleSet().empty()) {
        nbest = 5;
    }
    if (d->decoder_.decode(d->lattice_, d->graph_, nbest, state, max, min,
                           beamSize, frameSize)) {
        LIBIME_DEBUG() << "Decode: "
                       << std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::high_resolution_clock::now() - t0)
                              .count();
        std::unordered_map<std::string, size_t> dup;

        auto insertCandidate = [d, &dup](SentenceResult sentence) {
            auto sentenceString = sentence.toString();
            auto iter = dup.find(sentenceString);
            if (iter != dup.end()) {
                auto idx = iter->second;
                if (shouldReplaceCandidate(
                        d->candidates_[idx], sentence,
                        d->dict_.tableOptions().orderPolicy())) {
                    d->candidates_[idx] = std::move(sentence);
                }
            } else {
                d->candidates_.emplace_back(std::move(sentence));
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
        const float minDistance = TABLE_DEFAULT_MIN_DISTANCE;
        for (size_t i = 0, e = d->lattice_.sentenceSize(); i < e; i++) {
            const auto &sentence = d->lattice_.sentence(i);
            auto score = sentence.score();
            if (sentence.sentence().size() >= 1) {
                score = sentence.sentence().back()->score();
            }
            // Check the limit, or if there's no candidate.
            if (min - score < minDistance || candidates().size() == 0) {
                insertCandidate(sentence);
            }
        }
        LIBIME_DEBUG() << "Insert candidate: "
                       << std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::high_resolution_clock::now() - t0)
                              .count();
        int noSortLength =
            lastSegLength < d->dict_.tableOptions().noSortInputLength()
                ? lastSegLength
                : d->dict_.tableOptions().noSortInputLength();
        std::sort(d->candidates_.begin(), d->candidates_.end(),
                  TableCandidateCompare(d->dict_.tableOptions().orderPolicy(),
                                        noSortLength, lastSegLength));
        if (!d->candidates_.empty() &&
            TableCandidateCompare::isPinyin(d->candidates_[0])) {
            auto iter =
                std::find_if(d->candidates_.begin(), d->candidates_.end(),
                             [](const auto &cand) {
                                 return !TableCandidateCompare::isAuto(cand) &&
                                        !TableCandidateCompare::isPinyin(cand);
                             });
            // Make sure first is non pinyin/auto candidate.
            if (iter != d->candidates_.end()) {
                std::rotate(d->candidates_.begin(), iter, std::next(iter));
            }
        }
        LIBIME_DEBUG() << "Sort: "
                       << std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::high_resolution_clock::now() - t0)
                              .count();
        LIBIME_DEBUG() << "Number: " << d->candidates_.size();
    }

    // Run auto select for the second pass.
    // if number of candidate is 1, do auto select.
    if (d->dict_.tableOptions().autoSelect()) {
        if (d->hasOnlyOneAutoselectChoice() &&
            lastSegLength <= d->dict_.maxLength() &&
            !lengthLessThanLimit(lastSegLength,
                                 d->dict_.tableOptions().autoSelectLength())) {
            autoSelect();
        }
    }
}

TableContext::CandidateRange TableContext::candidates() const {
    FCITX_D();
    return boost::make_iterator_range(d->candidates_.begin(),
                                      d->candidates_.end());
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

size_t TableContext::selectedSegmentLength(size_t idx) const {
    FCITX_D();
    size_t prev = 0;
    if (idx > 0) {
        prev = d->selected_[idx - 1].back().offset_;
    }
    return d->selected_[idx].back().offset_ - prev;
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
    if (!d->dict_.tableOptions().learning()) {
        return;
    }

    if (!selected()) {
        return;
    }

    for (auto &s : d->selected_) {
        if (!d->learnWord(s)) {
            return;
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

void TableContext::learnAutoPhrase(std::string_view history) {
    FCITX_D();
    if (!d->dict_.tableOptions().learning() ||
        !fcitx::utf8::validate(history) ||
        d->dict_.tableOptions().autoPhraseLength() <= 1) {
        return;
    }

    auto range = fcitx::utf8::MakeUTF8CharRange(history);
    std::string code;
    for (auto iter = std::begin(range); iter != std::end(range); iter++) {
        auto charBegin = iter.charRange();
        auto length = fcitx::utf8::length(charBegin.first, history.end());
        if (length < 2 ||
            length > static_cast<size_t>(
                         d->dict_.tableOptions().autoPhraseLength())) {
            continue;
        }
        auto word =
            history.substr(std::distance(history.begin(), charBegin.first));
        LIBIME_TABLE_DEBUG()
            << "learnAutoPhrase " << word << " AutoPhraseLength: "
            << d->dict_.tableOptions().autoPhraseLength();
        ;
        if (!d->dict_.generate(word, code)) {
            continue;
        }
        auto wordFlag = d->dict_.wordExists(code, word);
        if (wordFlag == PhraseFlag::None || wordFlag == PhraseFlag::User) {
            continue;
        }
        auto insertResult = d->dict_.insert(code, word, PhraseFlag::Auto);
        LIBIME_TABLE_DEBUG() << insertResult;
    }
}

std::string TableContext::candidateHint(size_t idx, bool custom) const {
    FCITX_D();
    if (d->candidates_[idx].sentence().size() == 1) {
        auto p = d->candidates_[idx].sentence()[0];
        if (!p->word().empty()) {
            auto node = static_cast<const TableLatticeNode *>(p);
            if (node->flag() == PhraseFlag::Pinyin) {
                if (fcitx::utf8::length(p->word()) == 1) {
                    auto code = d->dict_.reverseLookup(node->word());
                    if (custom) {
                        return d->dict_.hint(code);
                    } else {
                        return code;
                    }
                }
            } else {
                std::string_view code = node->code();
                auto matchingKey = d->dict_.tableOptions().matchingKey();
                if (matchingKey) {
                    auto matchingKeyString =
                        fcitx::utf8::UCS4ToUTF8(matchingKey);
                    if (currentCode().find(matchingKeyString) !=
                        std::string::npos) {
                        return std::string{code};
                    }
                }
                code.remove_prefix(currentCode().size());
                if (custom) {
                    return d->dict_.hint(code);
                } else {
                    return std::string{code};
                }
            }
        }
    }
    return {};
}
} // namespace libime
