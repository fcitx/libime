/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
#include <fcitx-utils/charutils.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/utf8.h>
#include <iterator>
#include <unordered_set>

namespace libime {

struct SelectedPinyin {
    SelectedPinyin(size_t s, WordNode word, std::string encodedPinyin)
        : offset_(s), word_(std::move(word)),
          encodedPinyin_(std::move(encodedPinyin)) {}
    size_t offset_;
    WordNode word_;
    std::string encodedPinyin_;
};

class PinyinContextPrivate : public fcitx::QPtrHolder<PinyinContext> {
public:
    PinyinContextPrivate(PinyinContext *q, PinyinIME *ime)
        : QPtrHolder(q), ime_(ime), matchState_(q) {}

    std::vector<std::vector<SelectedPinyin>> selected_;

    bool sp_ = false;
    int maxSentenceLength_ = -1;
    PinyinIME *ime_;
    SegmentGraph segs_;
    Lattice lattice_;
    PinyinMatchState matchState_;
    std::vector<SentenceResult> candidates_;
    std::unordered_set<std::string> candidatesSet_;
    mutable bool candidatesToCursorNeedUpdate_ = true;
    mutable std::vector<SentenceResult> candidatesToCursor_;
    mutable std::unordered_set<std::string> candidatesToCursorSet_;
    std::vector<fcitx::ScopedConnection> conn_;

    size_t alignCursorToNextSegment() const {
        FCITX_Q();
        auto currentCursor = q->cursor();
        while (segs_.nodes(currentCursor).empty() &&
               currentCursor < q->size()) {
            currentCursor += 1;
        }
        return currentCursor;
    }

    bool needCandidatesToCursor() const {
        FCITX_Q();
        if (q->cursor() == q->selectedLength()) {
            return false;
        }

        return alignCursorToNextSegment() != q->size();
    }

    void clearCandidates() {
        candidates_.clear();
        candidatesToCursor_.clear();
        candidatesToCursorNeedUpdate_ = false;
        candidatesSet_.clear();
        candidatesToCursorSet_.clear();
    }

    void updateCandidatesToCursor() const {
        FCITX_Q();
        if (!candidatesToCursorNeedUpdate_) {
            return;
        }
        candidatesToCursorNeedUpdate_ = false;
        candidatesToCursor_.clear();
        candidatesToCursorSet_.clear();

        auto start = q->selectedLength();
        auto currentCursor = alignCursorToNextSegment();
        // Poke best sentence from lattice, ignore nbest option for now.
        auto nodeRange = lattice_.nodes(&segs_.node(currentCursor));
        if (!nodeRange.empty()) {
            candidatesToCursor_.push_back(nodeRange.front().toSentenceResult());
            candidatesToCursorSet_.insert(
                candidatesToCursor_.back().toString());
        }
        for (const auto &candidate : candidates_) {
            const auto &sentence = candidate.sentence();
            if (sentence.size() == 1) {
                if (sentence.back()->to()->index() + start > currentCursor) {
                    continue;
                }
                auto text = candidate.toString();
                if (candidatesToCursorSet_.count(text)) {
                    continue;
                }
                candidatesToCursor_.push_back(candidate);
                candidatesToCursorSet_.insert(std::move(text));
            } else if (sentence.size() > 1) {
                auto newSentence = sentence;
                while (!newSentence.empty() &&
                       newSentence.back()->to()->index() + start >
                           currentCursor) {
                    newSentence.pop_back();
                }
                if (!newSentence.empty()) {
                    SentenceResult partial(newSentence,
                                           newSentence.back()->score());
                    auto text = partial.toString();
                    if (candidatesToCursorSet_.count(text)) {
                        continue;
                    }
                    candidatesToCursor_.push_back(partial);
                    candidatesToCursorSet_.insert(std::move(text));
                }
            }
        }
    }

    void select(const SentenceResult &sentence) {
        FCITX_Q();
        auto offset = q->selectedLength();

        selected_.emplace_back();

        auto &selection = selected_.back();
        for (const auto &p : sentence.sentence()) {
            selection.emplace_back(
                offset + p->to()->index(),
                WordNode{p->word(), ime_->model()->index(p->word())},
                static_cast<const PinyinLatticeNode *>(p)->encodedPinyin());
        }
        // add some special code for handling separator at the end
        auto remain = std::string_view(q->userInput()).substr(offset);
        if (!remain.empty()) {
            if (std::all_of(remain.begin(), remain.end(),
                            [](char c) { return c == '\''; })) {
                selection.emplace_back(q->size(), WordNode("", 0), "");
            }
        }

        q->update();
    }
};

void matchPinyinCase(std::string_view ref, std::string &actualPinyin) {
    if (ref.size() != fcitx::utf8::length(actualPinyin)) {
        return;
    }

    auto iter = fcitx::utf8::MakeUTF8CharIterator(actualPinyin.begin(),
                                                  actualPinyin.end());
    for (size_t i = 0; i < ref.size(); ++i, ++iter) {
        if (fcitx::charutils::isupper(ref[i])) {
            auto charRange = iter.charRange();
            if (iter.charLength() == 1 &&
                fcitx::charutils::islower(iter.view()[0])) {
                *charRange.first = fcitx::charutils::toupper(*charRange.first);
            } else if (*iter == 0x00fc) {
                *charRange.first = 0xc3;
                *std::next(charRange.first) = 0x9c;
            }
        }
    }
}

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

void PinyinContext::setMaxSentenceLength(int length) {
    FCITX_D();
    d->maxSentenceLength_ = length;
    d->matchState_.clear();
}

int PinyinContext::maxSentenceLength() const {
    FCITX_D();
    return d->maxSentenceLength_;
}

bool PinyinContext::typeImpl(const char *s, size_t length) {
    FCITX_D();
    if (d->maxSentenceLength_ > 0 && !d->candidates_.empty()) {
        auto size = 0;
        for (const auto &s : d->candidates_[0].sentence()) {
            // This is pinyin length + 1
            auto segLength = s->path().size();
            size +=
                std::max(static_cast<decltype(segLength)>(1), segLength) - 1;
        }
        if (size > d->maxSentenceLength_) {
            return false;
        }
    }
    auto changed = cancelTill(cursor());
    changed = InputBuffer::typeImpl(s, length) || changed;
    if (changed) {
        update();
    }
    return changed;
}

void PinyinContext::erase(size_t from, size_t to) {
    if (from == to) {
        return;
    }

    // check if erase everything
    if (from == 0 && to >= size()) {
        FCITX_D();
        d->clearCandidates();
        d->selected_.clear();
        d->lattice_.clear();
        d->matchState_.clear();
        d->segs_ = SegmentGraph();
    } else {
        cancelTill(from);
    }
    InputBuffer::erase(from, to);

    if (!empty()) {
        update();
    }
}

void PinyinContext::setCursor(size_t pos) {
    FCITX_D();
    auto oldCursor = cursor();
    auto cancelled = cancelTill(pos);
    InputBuffer::setCursor(pos);
    if (cancelled) {
        update();
    } else {
        if (cursor() != oldCursor) {
            d->candidatesToCursorNeedUpdate_ = true;
        }
    }
}

int PinyinContext::pinyinBeforeCursor() const {
    FCITX_D();
    auto len = selectedLength();
    auto c = cursor();
    if (c < len) {
        return -1;
    }
    c -= len;
    if (!d->candidates_.empty()) {
        for (const auto &s : d->candidates_[0].sentence()) {
            for (auto iter = s->path().begin(),
                      end = std::prev(s->path().end());
                 iter < end; iter++) {
                auto from = (*iter)->index(), to = (*std::next(iter))->index();
                if (to >= c) {
                    return from + len;
                }
            }
        }
    }
    return -1;
}

int PinyinContext::pinyinAfterCursor() const {
    FCITX_D();
    auto len = selectedLength();
    auto c = cursor();
    if (c < len) {
        return -1;
    }
    c -= len;
    if (!d->candidates_.empty()) {
        for (const auto &s : d->candidates_[0].sentence()) {
            for (auto iter = s->path().begin(),
                      end = std::prev(s->path().end());
                 iter < end; iter++) {
                auto to = (*std::next(iter))->index();
                if (to > c) {
                    return to + len;
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

const std::unordered_set<std::string> &PinyinContext::candidateSet() const {
    FCITX_D();
    return d->candidatesSet_;
}

const std::vector<SentenceResult> &PinyinContext::candidatesToCursor() const {
    FCITX_D();
    if (!d->needCandidatesToCursor()) {
        return d->candidates_;
    }
    d->updateCandidatesToCursor();
    return d->candidatesToCursor_;
}

const std::unordered_set<std::string> &
PinyinContext::candidatesToCursorSet() const {
    FCITX_D();
    if (!d->needCandidatesToCursor()) {
        return d->candidatesSet_;
    }
    d->updateCandidatesToCursor();
    return d->candidatesToCursorSet_;
}

void PinyinContext::select(size_t idx) {
    FCITX_D();
    const auto &candidates = this->candidates();
    assert(idx < candidates.size());
    d->select(candidates[idx]);
}

void PinyinContext::selectCandidatesToCursor(size_t idx) {
    FCITX_D();
    const auto &candidates = this->candidatesToCursor();
    assert(idx < candidates.size());
    d->select(candidates[idx]);
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
    if (!d->selected_.empty()) {
        d->selected_.pop_back();

        // There is no point to reuse any existing matching state.
        // For example, cancel from (tao zi) tao zi to  tao zi tao zi.
        // Even if they share the same prefix, the start state won't be same.
        d->lattice_.clear();
        d->matchState_.clear();
        d->segs_ = SegmentGraph();
    }
    update();
}

State PinyinContext::state() const {
    FCITX_D();
    auto *model = d->ime_->model();
    State state = model->nullState();
    if (!d->selected_.empty()) {
        for (const auto &s : d->selected_) {
            for (const auto &item : s) {
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
    if (empty()) {
        clear();
        return;
    }

    if (selected()) {
        d->clearCandidates();
    } else {
        size_t start = 0;
        auto *model = d->ime_->model();
        State state = model->nullState();
        if (!d->selected_.empty()) {
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
                userInput().substr(start), *spProfile, d->ime_->fuzzyFlags());
        } else {
            newGraph = PinyinEncoder::parseUserPinyin(userInput().substr(start),
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

        d->clearCandidates();
        for (size_t i = 0, e = d->lattice_.sentenceSize(); i < e; i++) {
            d->candidates_.push_back(d->lattice_.sentence(i));
            d->candidatesSet_.insert(d->candidates_.back().toString());
        }

        const auto *bos = &graph.start();

        auto beginSize = d->candidates_.size();
        for (size_t i = graph.size(); i > 0; i--) {
            float min = 0;
            float max = -std::numeric_limits<float>::max();
            auto distancePenalty = d->ime_->model()->unknownPenalty() /
                                   PINYIN_DISTANCE_PENALTY_FACTOR;
            for (const auto &graphNode : graph.nodes(i)) {
                auto distance = graph.distanceToEnd(graphNode);
                auto adjust = static_cast<float>(distance) * distancePenalty;
                for (const auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                    if (latticeNode.from() == bos) {
                        if (!d->ime_->model()->isNodeUnknown(latticeNode)) {
                            if (latticeNode.score() < min) {
                                min = latticeNode.score();
                            }
                            if (latticeNode.score() > max) {
                                max = latticeNode.score();
                            }
                        }
                        if (d->candidatesSet_.count(latticeNode.word())) {
                            continue;
                        }
                        d->candidates_.push_back(
                            latticeNode.toSentenceResult(adjust));
                        d->candidatesSet_.insert(latticeNode.word());
                    }
                }
            }
            for (const auto &graphNode : graph.nodes(i)) {
                auto distance = graph.distanceToEnd(graphNode);
                auto adjust = static_cast<float>(distance) * distancePenalty;
                for (const auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                    if (latticeNode.from() != bos &&
                        latticeNode.score() > min &&
                        latticeNode.score() + d->ime_->maxDistance() > max) {
                        auto fullWord = latticeNode.fullWord();
                        if (d->candidatesSet_.count(fullWord)) {
                            continue;
                        }
                        d->candidates_.push_back(
                            latticeNode.toSentenceResult(adjust));
                        d->candidatesSet_.insert(fullWord);
                    }
                }
            }
        }
        std::sort(d->candidates_.begin() + beginSize, d->candidates_.end(),
                  std::greater<SentenceResult>());
        d->candidatesToCursorNeedUpdate_ = true;
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

    if (!d->selected_.empty()) {
        if (d->selected_.back().back().offset_ == size()) {
            return true;
        }
    }

    return false;
}

std::string PinyinContext::selectedSentence() const {
    FCITX_D();
    std::string ss;
    for (const auto &s : d->selected_) {
        for (const auto &item : s) {
            ss += item.word_.word();
        }
    }
    return ss;
}

size_t PinyinContext::selectedLength() const {
    FCITX_D();
    if (!d->selected_.empty()) {
        return d->selected_.back().back().offset_;
    }
    return 0;
}

std::string PinyinContext::preedit() const {
    return preedit(ime()->preeditMode());
}

std::pair<std::string, size_t> PinyinContext::preeditWithCursor() const {
    return preeditWithCursor(ime()->preeditMode());
}

std::string PinyinContext::preedit(PinyinPreeditMode mode) const {
    return preeditWithCursor(mode).first;
}

std::pair<std::string, size_t>
PinyinContext::preeditWithCursor(PinyinPreeditMode mode) const {
    FCITX_D();
    std::string ss = selectedSentence();
    const auto len = selectedLength();
    auto c = cursor();
    size_t actualCursor = ss.size();
    // should not happen
    if (c < len) {
        c = len;
    }

    auto resultSize = ss.size();

    if (!d->candidates_.empty()) {
        bool first = true;
        for (const auto &node : d->candidates_[0].sentence()) {
            for (auto iter = node->path().begin(),
                      end = std::prev(node->path().end());
                 iter < end; iter++) {
                if (!first) {
                    ss += " ";
                    resultSize += 1;
                } else {
                    first = false;
                }
                auto from = (*iter)->index(), to = (*std::next(iter))->index();
                size_t cursorInPinyin = c - from - len;
                const size_t startPivot = resultSize;
                auto pinyin = d->segs_.segment(from, to);
                MatchedPinyinSyllables syls;
                if (mode == PinyinPreeditMode::Pinyin) {
                    // The reason that we don't use fuzzy flag from option is
                    // that we'd like to keep the preedit as is. Otherwise,
                    // "qign" would be displayed as "qing", which would be
                    // confusing to user about what is actually being typed.
                    syls = useShuangpin()
                               ? PinyinEncoder::shuangpinToSyllables(
                                     pinyin, *ime()->shuangpinProfile(),
                                     PinyinFuzzyFlag::None)
                               : PinyinEncoder::stringToSyllables(
                                     pinyin, PinyinFuzzyFlag::None);
                }
                std::string actualPinyin;
                if (!syls.empty() && !syls.front().second.empty()) {
                    std::string_view candidatePinyin =
                        static_cast<const PinyinLatticeNode *>(node)
                            ->encodedPinyin();
                    auto nthPinyin = std::distance(node->path().begin(), iter);
                    PinyinInitial bestInitial = syls[0].first;
                    PinyinFinal bestFinal = syls[0].second[0].first;

                    // Try to match the candidate syllables from all possible
                    // none-fuzzy possible syls.
                    if (static_cast<size_t>(nthPinyin * 2 + 2) <=
                        candidatePinyin.size()) {
                        auto candidateInitial = static_cast<PinyinInitial>(
                            candidatePinyin[nthPinyin * 2]);
                        auto candidateFinal = static_cast<PinyinFinal>(
                            candidatePinyin[nthPinyin * 2 + 1]);

                        bool found = false;
                        for (const auto &initial : syls) {
                            for (const auto &[final, fuzzy] : initial.second) {
                                if (fuzzy) {
                                    continue;
                                }
                                if (candidateInitial == initial.first &&
                                    (final == PinyinFinal::Invalid ||
                                     candidateFinal == final)) {
                                    bestInitial = initial.first;
                                    if (final != PinyinFinal::Invalid) {
                                        bestFinal = final;
                                    }
                                    found = true;
                                    break;
                                }
                            }
                            if (found) {
                                break;
                            }
                        }
                    }

                    actualPinyin = PinyinEncoder::initialFinalToPinyinString(
                        bestInitial, bestFinal);
                    if (!useShuangpin()) {
                        matchPinyinCase(pinyin, actualPinyin);
                    }
                }
                if (!actualPinyin.empty()) {
                    if (c > from + len && c <= to + len) {
                        if (useShuangpin()) {
                            switch (cursorInPinyin) {
                            case 0:
                                break;
                            case 1:
                                if (pinyin.size() == 2 &&
                                    syls[0].first == PinyinInitial::Zero) {
                                    actualPinyin = fcitx::stringutils::concat(
                                        "_", actualPinyin);
                                }
                                // Zero case, we just append one.
                                if (syls[0].first != PinyinInitial::Zero) {
                                    cursorInPinyin =
                                        PinyinEncoder::initialToString(
                                            syls[0].first)
                                            .size();
                                }
                                break;
                            default:
                                cursorInPinyin = actualPinyin.size();
                                break;
                            }
                        } else {
                            cursorInPinyin =
                                std::min(actualPinyin.size(), cursorInPinyin);
                            cursorInPinyin = fcitx::utf8::ncharByteLength(
                                actualPinyin.begin(), cursorInPinyin);
                        }
                    }
                    ss.append(actualPinyin);
                    resultSize += actualPinyin.size();
                } else {
                    ss.append(pinyin.data(), pinyin.size());
                    resultSize += pinyin.size();
                }
                if (c > from + len && c <= to + len) {
                    actualCursor = startPivot + cursorInPinyin;
                }
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
    for (const auto &s : d->selected_) {
        for (const auto &item : s) {
            if (!item.word_.word().empty()) {
                newSentence.push_back(item.word_.word());
            }
        }
    }
    return newSentence;
}

std::vector<std::pair<std::string, std::string>>
PinyinContext::selectedWordsWithPinyin() const {
    FCITX_D();
    std::vector<std::pair<std::string, std::string>> newSentence;
    for (const auto &s : d->selected_) {
        for (const auto &item : s) {
            if (!item.word_.word().empty()) {
                newSentence.emplace_back(item.word_.word(),
                                         item.encodedPinyin_);
            }
        }
    }
    return newSentence;
}

std::string PinyinContext::selectedFullPinyin() const {
    FCITX_D();
    std::string pinyin;
    for (const auto &s : d->selected_) {
        for (const auto &item : s) {
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
    return candidateFullPinyin(d->candidates_[idx]);
}

std::string
PinyinContext::candidateFullPinyin(const SentenceResult &candidate) const {
    std::string pinyin;
    for (const auto &node : candidate.sentence()) {
        if (!node->word().empty()) {
            if (!pinyin.empty()) {
                pinyin.push_back('\'');
            }
            pinyin += PinyinEncoder::decodeFullPinyin(
                static_cast<const PinyinLatticeNode *>(node)->encodedPinyin());
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
                    // Non pinyin word. Skip it.
                    if (item.encodedPinyin_.empty()) {
                        return;
                    }
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
    // don't learn existing word.
    if (d->selected_.size() == 1 && d->selected_[0].size() == 1) {
        return false;
    }
    for (auto &s : d->selected_) {
        bool first = true;
        for (auto &item : s) {
            if (!item.word_.word().empty()) {
                // We can't learn non pinyin word.
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
} // namespace libime
