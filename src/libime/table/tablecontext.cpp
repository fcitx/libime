/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "tablecontext.h"
#include "constants.h"
#include "libime/core/historybigram.h"
#include "libime/core/inputbuffer.h"
#include "libime/core/languagemodel.h"
#include "libime/core/lattice.h"
#include "libime/core/segmentgraph.h"
#include "libime/core/userlanguagemodel.h"
#include "libime/core/utils.h"
#include "libime/table/tablebaseddictionary.h"
#include "log.h"
#include "tablebaseddictionary_p.h"
#include "tabledecoder.h"
#include "tableoptions.h"
#include <algorithm>
#include <boost/range/iterator_range_core.hpp>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fcitx-utils/inputbuffer.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/utf8.h>
#include <iterator>
#include <limits>
#include <memory>
#include <regex>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace libime {

namespace {

size_t sentenceCodeLength(const SentenceResult &sentence) {
    const auto *node =
        static_cast<const TableLatticeNode *>(sentence.sentence()[0]);
    return node->codeLength();
}

template <OrderPolicy policy>
struct TableCandidateCompare {
    TableCandidateCompare(int noSortInputLength, bool sortByCodeLength)
        : noSortInputLength_(noSortInputLength),
          sortByCodeLength_(sortByCodeLength) {}

    // Larger index should be put ahead.
    static int64_t index(const SentenceResult &sentence) {
        const auto *const node =
            static_cast<const TableLatticeNode *>(sentence.sentence()[0]);
        if (node->flag() == PhraseFlag::User) {
            return node->index();
        }
        return -static_cast<int64_t>(node->index());
    }

    bool operator()(const SentenceResult &lhs,
                    const SentenceResult &rhs) const {
        const bool lIsAuto = TableContext::isAuto(lhs);
        const bool rIsAuto = TableContext::isAuto(rhs);
        if (lIsAuto != rIsAuto) {
            return lIsAuto < rIsAuto;
        }
        // non-auto word
        if (!lIsAuto) {
            const bool lIsPinyin = TableContext::isPinyin(lhs);
            const bool rIsPinyin = TableContext::isPinyin(rhs);
            const auto lLength = sentenceCodeLength(lhs);
            const auto rLength = sentenceCodeLength(rhs);
            const bool lShort =
                static_cast<int>(lLength) <= noSortInputLength_ && !lIsPinyin;
            const bool rShort =
                static_cast<int>(rLength) <= noSortInputLength_ && !rIsPinyin;
            if (lShort != rShort) {
                return lShort > rShort;
            }
            // Always sort result by code length.
            if (sortByCodeLength_ && lLength != rLength) {
                return lLength < rLength;
            }

            if (lShort) {
                return index(lhs) > index(rhs);
            }

            if constexpr (policy == OrderPolicy::No ||
                          policy == OrderPolicy::Fast) {
                return index(lhs) > index(rhs);
            } else if constexpr (policy == OrderPolicy::Freq) {
                float lScore = lhs.score();
                float rScore = rhs.score();
                if (lScore != rScore) {
                    return lScore > rScore;
                }
                return index(lhs) > index(rhs);
            }
            return false;
        }

        return lhs.score() > rhs.score();
    }

private:
    const int noSortInputLength_;
    const bool sortByCodeLength_;
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
        auto oldCode = sentenceCodeLength(newSentence);
        auto newCode = sentenceCodeLength(oldSentence);

        if (oldCode != newCode) {
            return oldCode < newCode;
        }

        const auto *newNode =
            static_cast<const TableLatticeNode *>(newSentence.sentence()[0]);
        switch (policy) {
        case OrderPolicy::No:
            if (newNode->flag() != PhraseFlag::User) {
                return true;
            }
            break;
        case OrderPolicy::Freq:
            if (newSentence.score() != oldSentence.score()) {
                return newSentence.score() > oldSentence.score();
            }
            [[fallthrough]];
        case OrderPolicy::Fast:
            if (newNode->flag() == PhraseFlag::User) {
                return true;
            }
            break;
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
        if (candidates_.empty()) {
            return false;
        }
        return !TableContext::isAuto(candidates_[0]);
    };

    // sort should already happened at this point.
    bool hasOnlyOneAutoselectChoice() const {
        if (!canDoAutoSelect()) {
            return false;
        }
        if (candidates_.size() != 1) {
            return false;
        }

        if (candidates_[0].sentence().size() != 1) {
            return false;
        }
        FCITX_Q();
        return libime::TableContext::code(candidates_[0]) == q->currentCode() &&
               (!dict_.tableOptions().exactMatch() ||
                dict_.hasOneMatchingWord(q->currentCode()));
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
        if (!selected_.empty()) {
            return selected_.back().back().offset_;
        }
        return 0;
    }

    void cancel() {
        if (!selected_.empty()) {
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
            const auto &select = selection[0];
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

    bool checkAutoSelect() const {
        auto lastSegLength = fcitx::utf8::length(graph_.data());
        // Check by length
        if (dict_.tableOptions().autoSelectLength() &&
            !lengthLessThanLimit(lastSegLength,
                                 dict_.tableOptions().autoSelectLength())) {
            return true;
        }

        // Check by regex.
        return dict_.d_func()->autoSelectRegex_ &&
               std::regex_match(graph_.data(),
                                *dict_.d_func()->autoSelectRegex_,
                                std::regex_constants::match_default);
    }

    bool checkNoMatchAutoSelect() const {
        auto lastSegLength = fcitx::utf8::length(graph_.data());
        // Check by length
        if (dict_.tableOptions().noMatchAutoSelectLength() &&
            !lengthLessThanLimit(
                lastSegLength,
                dict_.tableOptions().noMatchAutoSelectLength())) {
            return true;
        }

        // Check by regex.
        return dict_.d_func()->noMatchAutoSelectRegex_ &&
               std::regex_match(graph_.data(),
                                *dict_.d_func()->noMatchAutoSelectRegex_,
                                std::regex_constants::match_default);
    }

    TableBasedDictionary &dict_;
    UserLanguageModel &model_;
    TableDecoder decoder_;
    Lattice lattice_;
    SegmentGraph graph_;
    std::vector<SentenceResult> candidates_;
    std::vector<std::vector<SelectedCode>> selected_;
    size_t autoSelectIndex_ = 0;
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
    for (const auto &p : d->candidates_[idx].sentence()) {
        const auto *node = static_cast<const TableLatticeNode *>(p);
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

    const auto &option = d->dict_.tableOptions();
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
            doAutoSelect || (d->checkNoMatchAutoSelect() &&
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

void TableContext::setAutoSelectIndex(size_t index) {
    FCITX_D();
    d->autoSelectIndex_ = index;
}

void TableContext::autoSelect() {
    FCITX_D();
    if (selected()) {
        return;
    }

    if (d->canDoAutoSelect()) {
        auto selectIndex = d->autoSelectIndex_;
        d->autoSelectIndex_ = 0;
        if (selectIndex >= candidates().size()) {
            selectIndex = 0;
        }
        select(selectIndex);
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
    d->autoSelectIndex_ = 0;
    if (empty()) {
        return;
    }

    if (selected()) {
        d->resetMatchingState();
        return;
    }

    d->lattice_.clear();
    State state = d->currentState();

    auto t0 = std::chrono::high_resolution_clock::now();
    decltype(t0) t1;
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
        t1 = std::chrono::high_resolution_clock::now();
        LIBIME_TABLE_DEBUG()
            << "Decode: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0)
                   .count();
        t0 = t1;
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
        const SegmentGraphNode *bos = &graph.start();
        const SegmentGraphNode *eos = &graph.end();
        constexpr float pinyinPenalty = -0.5;
        for (const auto &latticeNode : d->lattice_.nodes(eos)) {
            if (latticeNode.from() == bos && latticeNode.to() == eos) {
                auto sentence = latticeNode.toSentenceResult();
                if (TableContext::isPinyin(sentence)) {
                    sentence.adjustScore(pinyinPenalty);
                }
                insertCandidate(std::move(sentence));
            }
        }

        float min = 0;
        for (const auto &cand : d->candidates_) {
            min = std::min(min, cand.score());
        }

        // FIXME: add an option.
        const float minDistance = TABLE_DEFAULT_MIN_DISTANCE;
        for (size_t i = 0, e = d->lattice_.sentenceSize(); i < e; i++) {
            auto sentence = d->lattice_.sentence(i);
            if (TableContext::isPinyin(sentence)) {
                sentence.adjustScore(pinyinPenalty);
            }
            auto score = sentence.score();
            if (!sentence.sentence().empty()) {
                score = sentence.sentence().back()->score();
            }
            // Check the limit, or if there's no candidate.
            if (min - score < minDistance || candidates().empty()) {
                insertCandidate(std::move(sentence));
            }
        }
        t1 = std::chrono::high_resolution_clock::now();
        LIBIME_TABLE_DEBUG()
            << "Insert candidate: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0)
                   .count();
        t0 = t1;
        int noSortLength =
            lastSegLength < d->dict_.tableOptions().noSortInputLength()
                ? lastSegLength
                : d->dict_.tableOptions().noSortInputLength();

        switch (d->dict_.tableOptions().orderPolicy()) {
        case OrderPolicy::No:
            std::sort(
                d->candidates_.begin(), d->candidates_.end(),
                TableCandidateCompare<OrderPolicy::No>(
                    noSortLength, d->dict_.tableOptions().sortByCodeLength()));
            break;
        case OrderPolicy::Fast:
            std::sort(
                d->candidates_.begin(), d->candidates_.end(),
                TableCandidateCompare<OrderPolicy::Fast>(
                    noSortLength, d->dict_.tableOptions().sortByCodeLength()));
            break;
        case OrderPolicy::Freq:
            std::sort(
                d->candidates_.begin(), d->candidates_.end(),
                TableCandidateCompare<OrderPolicy::Freq>(
                    noSortLength, d->dict_.tableOptions().sortByCodeLength()));
            break;
        }
        if (!d->candidates_.empty() && isPinyin(d->candidates_[0])) {
            auto iter =
                std::find_if(d->candidates_.begin(), d->candidates_.end(),
                             [](const auto &cand) {
                                 return !isAuto(cand) && !isPinyin(cand);
                             });
            // Make sure first is non pinyin/auto candidate.
            if (iter != d->candidates_.end()) {
                std::rotate(d->candidates_.begin(), iter, std::next(iter));
            }
        }

        t1 = std::chrono::high_resolution_clock::now();
        LIBIME_TABLE_DEBUG()
            << "Sort: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0)
                   .count();
        LIBIME_TABLE_DEBUG() << "Number: " << d->candidates_.size();
    };
    // Run auto select for the second pass.
    // if number of candidate is 1, do auto select.
    if (d->dict_.tableOptions().autoSelect()) {
        if (d->hasOnlyOneAutoselectChoice() &&
            lastSegLength <= d->dict_.maxLength() && d->checkAutoSelect()) {
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
    for (const auto &s : d->selected_) {
        for (const auto &item : s) {
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
    for (const auto &item : d->selected_[idx]) {
        if (!item.commit_) {
            commit = false;
        }
        result += item.word_.word();
    }
    return {std::move(result), commit};
}

std::string TableContext::selectedCode(size_t idx) const {
    FCITX_D();
    std::string result;
    for (const auto &item : d->selected_[idx]) {
        result += item.code_;
    }
    return result;
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

    if (d->selected_.empty()) {
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
    if (!newSentence.empty()) {
        d->model_.history().add(newSentence);
    }
}

void TableContext::learnLast() {
    FCITX_D();
    if (!d->dict_.tableOptions().learning() || d->selected_.empty()) {
        return;
    }

    if (!d->learnWord(d->selected_.back())) {
        return;
    }

    std::vector<std::string> newSentence;
    std::string word;
    for (auto &item : d->selected_.back()) {
        if (!item.commit_) {
            word.clear();
            return;
        }
        word += item.word_.word();
    }
    if (!word.empty()) {
        newSentence.emplace_back(std::move(word));
    }
    if (!newSentence.empty()) {
        d->model_.history().add(newSentence);
    }
}

void TableContext::learnAutoPhrase(std::string_view history) {
    learnAutoPhrase(history, {});
}

void TableContext::learnAutoPhrase(std::string_view history,
                                   const std::vector<std::string> &hints) {
    FCITX_D();
    if (!d->dict_.tableOptions().learning() ||
        !fcitx::utf8::validate(history) ||
        d->dict_.tableOptions().autoPhraseLength() <= 1) {
        return;
    }

    auto range = fcitx::utf8::MakeUTF8CharRange(history);
    std::string code;

    std::vector<std::string> currentHints;
    size_t i = 0;
    for (auto iter = std::begin(range); iter != std::end(range); iter++, i++) {
        auto charBegin = iter.charRange();
        auto length = fcitx::utf8::length(charBegin.first, history.end());
        if (length < 2 ||
            length > static_cast<size_t>(
                         d->dict_.tableOptions().autoPhraseLength())) {
            continue;
        }
        // Make a substring from current char.
        auto word =
            history.substr(std::distance(history.begin(), charBegin.first));
        auto begin = hints.end();
        if (hints.size() > i) {
            begin = std::next(hints.begin(), i);
        }
        currentHints.assign(begin, hints.end());
        if (!d->dict_.generateWithHint(word, currentHints, code)) {
            continue;
        }
        auto wordFlag = d->dict_.wordExists(code, word);
        if (wordFlag == PhraseFlag::None || wordFlag == PhraseFlag::User) {
            continue;
        }
        auto insertResult = d->dict_.insert(code, word, PhraseFlag::Auto);
        LIBIME_TABLE_DEBUG() << "learnAutoPhrase " << word << " " << code
                             << " AutoPhraseLength: "
                             << d->dict_.tableOptions().autoPhraseLength()
                             << " success: " << insertResult;
    }
}

std::string TableContext::candidateHint(size_t idx, bool custom) const {
    FCITX_D();
    if (d->candidates_[idx].sentence().size() == 1) {
        const auto *p = d->candidates_[idx].sentence()[0];
        if (!p->word().empty()) {
            const auto *node = static_cast<const TableLatticeNode *>(p);
            if (node->flag() == PhraseFlag::Pinyin) {
                if (fcitx::utf8::length(p->word()) == 1) {
                    auto code = d->dict_.reverseLookup(node->word());
                    if (custom) {
                        return d->dict_.hint(code);
                    }
                    return code;
                }
            } else {
                std::string_view code = node->code();
                auto matchingKey = d->dict_.tableOptions().matchingKey();
                // If we're not using matching key remove the prefix.
                // Otherwise show the full code.
                if (!matchingKey || (currentCode().find(fcitx::utf8::UCS4ToUTF8(
                                         matchingKey)) == std::string::npos)) {
                    code.remove_prefix(currentCode().size());
                }
                if (custom) {
                    return d->dict_.hint(code);
                }
                return std::string{code};
            }
        }
    }
    return {};
}

std::string TableContext::code(const SentenceResult &sentence) {
    if (sentence.size() == 1) {
        const auto *node =
            static_cast<const TableLatticeNode *>(sentence.sentence()[0]);
        return node->code();
    }
    return "";
}

PhraseFlag TableContext::flag(const SentenceResult &sentence) {
    if (sentence.size() == 1) {
        const auto *node =
            static_cast<const TableLatticeNode *>(sentence.sentence()[0]);
        return node->flag();
    }
    return PhraseFlag::Auto;
}

bool TableContext::isPinyin(const SentenceResult &sentence) {
    return sentence.size() == 1 && flag(sentence) == PhraseFlag::Pinyin;
}

bool TableContext::isAuto(const SentenceResult &sentence) {
    return sentence.size() != 1 || flag(sentence) == PhraseFlag::Auto;
}

} // namespace libime
