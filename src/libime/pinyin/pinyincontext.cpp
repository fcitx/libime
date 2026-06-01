/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "pinyincontext.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <boost/container_hash/hash.hpp>
#include <fcitx-utils/charutils.h>
#include <fcitx-utils/inputbuffer.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/signals.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/utf8.h>
#include "libime/core/historybigram.h"
#include "libime/core/inputbuffer.h"
#include "libime/core/languagemodel.h"
#include "libime/core/lattice.h"
#include "libime/core/segmentgraph.h"
#include "libime/core/userlanguagemodel.h"
#include "libime/pinyin/constants.h"
#include "pinyindecoder.h"
#include "pinyindecoder_p.h"
#include "pinyinencoder.h"
#include "pinyinime.h"
#include "pinyinmatchstate.h"

namespace libime {

namespace {
enum class LearnWordResult {
    Normal,  /// word is consisted all from regular word from dict.
    Custom,  /// word is consisted with custom word (e.g. symbol replacement).
    Ignored, /// not learned as word.
};

enum class SelectedPinyinType {
    Normal,
    Custom,
    Separator,
};

struct SelectedPinyin {
    SelectedPinyin(size_t s, PinyinWordNode word, SelectedPinyinType type)
        : offset_(s), word_(std::move(word)), type_(type) {}

    const std::string &encodedPinyin() const { return word_.encodedPinyin(); }

    size_t offset_;
    PinyinWordNode word_;
    SelectedPinyinType type_;
};

struct CandidateDedupKey {
    std::string text_;
    size_t end_ = 0;

    bool operator==(const CandidateDedupKey &other) const {
        return text_ == other.text_ && end_ == other.end_;
    }
};

struct CandidateDedupKeyHash {
    size_t operator()(const CandidateDedupKey &key) const {
        size_t seed = std::hash<std::string>()(key.text_);
        boost::hash_combine(seed, key.end_);
        return seed;
    }
};

CandidateDedupKey candidateDedupKey(const SentenceResult &candidate) {
    return {.text_ = candidate.toString(),
            .end_ = candidate.sentence().empty()
                        ? 0
                        : candidate.sentence().back()->to()->index()};
}

} // namespace

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
    std::list<PinyinWordNode> contextWords_;

    size_t alignCursorToNextSegment() const {
        FCITX_Q();
        auto currentCursor = q->cursor();
        auto start = q->selectedLength();
        if (currentCursor < start) {
            return start;
        }
        while (segs_.nodes(currentCursor - start).empty() &&
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

        std::unordered_map<CandidateDedupKey, size_t, CandidateDedupKeyHash>
            duplicateCandidates;
        auto insertCandidate = [this, &duplicateCandidates](
                                   SentenceResult candidate) {
            auto key = candidateDedupKey(candidate);
            auto iter = duplicateCandidates.find(key);
            if (iter != duplicateCandidates.end()) {
                auto &oldCandidate = candidatesToCursor_[iter->second];
                if (candidate.score() > oldCandidate.score()) {
                    oldCandidate = std::move(candidate);
                }
                return;
            }

            candidatesToCursor_.push_back(std::move(candidate));
            duplicateCandidates.emplace(key, candidatesToCursor_.size() - 1);
            candidatesToCursorSet_.insert(std::move(key.text_));
        };

        auto start = q->selectedLength();
        auto currentCursor = alignCursorToNextSegment();
        // Poke best sentence from lattice, ignore nbest option for now.
        auto nodeRange = lattice_.nodes(&segs_.node(currentCursor - start));
        if (!nodeRange.empty()) {
            insertCandidate(nodeRange.front().toSentenceResult());
        }
        for (const auto &candidate : candidates_) {
            const auto &sentence = candidate.sentence();
            if (sentence.size() == 1) {
                if (sentence.back()->to()->index() + start > currentCursor) {
                    continue;
                }
                insertCandidate(candidate);
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
                    insertCandidate(std::move(partial));
                }
            }
        }
    }

    template <typename FillSentence>
    void selectHelper(const FillSentence &fillSentence) {
        FCITX_Q();
        selected_.emplace_back();

        auto &selection = selected_.back();
        fillSentence(selection);
        // add some special code for handling separator at the end
        auto remain =
            std::string_view(q->userInput()).substr(q->selectedLength());
        if (!remain.empty()) {
            if (std::all_of(remain.begin(), remain.end(),
                            [](char c) { return c == '\''; })) {
                selection.emplace_back(q->size(), PinyinWordNode({}, 0),
                                       SelectedPinyinType::Separator);
            }
        }

        q->update();
    }

    void select(const SentenceResult &sentence) {
        FCITX_Q();
        auto offset = q->selectedLength();
        selectHelper([offset, &sentence,
                      this](std::vector<SelectedPinyin> &selection) {
            for (const auto &p : sentence.sentence()) {
                selection.emplace_back(
                    offset + p->to()->index(),
                    PinyinWordNode{
                        {p->word(), p->as<PinyinLatticeNode>().encodedPinyin()},
                        ime_->model()->index(p->word())},

                    SelectedPinyinType::Normal);
            }
        });
    }

    void selectCustom(size_t inputLength, std::string_view segment,
                      std::string_view encodedPinyin) {
        FCITX_Q();
        auto offset = q->selectedLength();
        selectHelper([this, offset, &segment, inputLength,
                      &encodedPinyin](std::vector<SelectedPinyin> &selection) {
            auto index = ime_->model()->index(segment);
            selection.emplace_back(
                offset + inputLength,
                PinyinWordNode{{segment, encodedPinyin}, index},
                SelectedPinyinType::Custom);
        });
    }

    std::tuple<LearnWordResult, std::string> learnWord() {
        std::string ss;
        std::string pinyin;
        if (selected_.empty()) {
            return {LearnWordResult::Ignored, ""};
        }
        // don't learn existing word.
        if (selected_.size() == 1 && selected_[0].size() == 1) {
            return {LearnWordResult::Ignored, ""};
        }
        // Validate the learning word.
        // All single || custom || length <= 4
        bool hasCustom = false;
        size_t totalPinyinLength = 0;
        bool isAllSingleWord = true;
        for (auto &s : selected_) {
            isAllSingleWord =
                isAllSingleWord &&
                (s.empty() || (s.size() == 1 &&
                               (s[0].type_ == SelectedPinyinType::Separator ||
                                s[0].encodedPinyin().size() == 2)));
            for (auto &item : s) {
                if (item.type_ == SelectedPinyinType::Separator) {
                    continue;
                }
                if (item.type_ == SelectedPinyinType::Custom) {
                    hasCustom = true;
                }
                // We can't learn non pinyin word.
                if (item.encodedPinyin().empty() ||
                    item.encodedPinyin().size() % 2 != 0) {
                    return {LearnWordResult::Ignored, ""};
                }
                totalPinyinLength += item.encodedPinyin().size() / 2;
            }
        }

        FCITX_Q();
        if (!hasCustom) {
            if ((!isAllSingleWord && totalPinyinLength > 4)) {
                return {LearnWordResult::Ignored, ""};
            }
            if (ime_->model()->containsNonUnigram(q->selectedWords())) {
                return {LearnWordResult::Ignored, ""};
            }
        }

        for (auto &s : selected_) {
            for (auto &item : s) {
                if (item.type_ == SelectedPinyinType::Separator) {
                    continue;
                }
                assert(!item.encodedPinyin().empty());
                assert(item.encodedPinyin().size() % 2 == 0);
                ss += item.word_.word();
                if (!pinyin.empty()) {
                    pinyin.push_back('\'');
                }
                pinyin += PinyinEncoder::decodeFullPinyin(item.encodedPinyin());
            }
        }

        if (auto opt = ime_->dict()->lookupWord(PinyinDictionary::UserDict,
                                                pinyin, ss)) {
            return {LearnWordResult::Ignored, ""};
        }

        ime_->dict()->addWord(PinyinDictionary::UserDict, pinyin, ss,
                              hasCustom ? -1 : 0);

        auto encodedPinyin = PinyinEncoder::encodeFullPinyinWithFlags(
            pinyin, PinyinFuzzyFlag::VE_UE);

        return {hasCustom ? LearnWordResult::Custom : LearnWordResult::Normal,
                std::string(encodedPinyin.data(), encodedPinyin.size())};
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
                auto from = (*iter)->index();
                auto to = (*std::next(iter))->index();
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

void PinyinContext::selectCustom(size_t inputLength, std::string_view segment,
                                 std::string_view encodedPinyin) {
    FCITX_D();
    if (inputLength == 0 && selectedLength() + inputLength > size()) {
        throw std::out_of_range("Invalid input length");
    }
    if (encodedPinyin.size() % 2 != 0) {
        throw std::invalid_argument("Invalid encoded pinyin");
    }
    d->selectCustom(inputLength, segment, encodedPinyin);
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
    for (const auto &word : d->contextWords_) {
        State temp;
        model->score(state, word, temp);
        state = std::move(temp);
    }
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
        State state = this->state();
        if (!d->selected_.empty()) {
            start = d->selected_.back().back().offset_;
        }
        SegmentGraph newGraph;
        if (auto spProfile = d->matchState_.shuangpinProfile()) {
            newGraph = PinyinEncoder::parseUserShuangpin(
                userInput().substr(start), *spProfile, d->ime_->fuzzyFlags());
        } else {
            newGraph = PinyinEncoder::parseUserPinyin(
                userInput().substr(start), d->ime_->correctionProfile().get(),
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

        // Add n-best result.
        for (size_t i = 0, e = d->lattice_.sentenceSize(); i < e; i++) {
            d->candidates_.push_back(d->lattice_.sentence(i));
        }

        const auto *bos = &graph.start();

        auto beginSize = d->candidates_.size();
        for (size_t i = graph.size(); i > 0; i--) {
            float min = 0;
            float max = -std::numeric_limits<float>::max();
            auto distancePenalty = d->ime_->model()->unknownPenalty() /
                                   PINYIN_DISTANCE_PENALTY_FACTOR;

            // Enumerate over all the lattice node, if from == bos, this is
            // a dictionary word match.
            // Add all words that does not contain pinyin correction.
            for (const auto &graphNode : graph.nodes(i)) {
                auto distance = graph.distanceToEnd(graphNode);
                auto adjust = static_cast<float>(distance) * distancePenalty;
                for (const auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                    if (latticeNode.from() == bos &&
                        !static_cast<const PinyinLatticeNode &>(latticeNode)
                             .isCorrection()) {
                        if (!d->ime_->model()->isNodeUnknown(latticeNode)) {
                            min = std::min(latticeNode.score(), min);
                            max = std::max(latticeNode.score(), max);
                        }
                        d->candidates_.push_back(
                            latticeNode.toSentenceResult(adjust));
                    }
                }
            }

            // Filter correction word based on score
            for (const auto &graphNode : graph.nodes(i)) {
                auto distance = graph.distanceToEnd(graphNode);
                auto adjust = static_cast<float>(distance) * distancePenalty;
                for (const auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                    if (latticeNode.from() == bos &&
                        static_cast<const PinyinLatticeNode &>(latticeNode)
                            .isCorrection()) {
                        if ((latticeNode.score() > min &&
                             latticeNode.score() + d->ime_->maxDistance() >
                                 max) ||
                            static_cast<const PinyinLatticeNode &>(latticeNode)
                                    .encodedPinyin()
                                    .size() <= 2) {
                            d->candidates_.push_back(
                                latticeNode.toSentenceResult(adjust));
                        }
                    }
                }
            }

            // This part is the phrase that's constructable from lattice.
            for (const auto &graphNode : graph.nodes(i)) {
                auto distance = graph.distanceToEnd(graphNode);
                auto adjust = static_cast<float>(distance) * distancePenalty;
                for (const auto &latticeNode : d->lattice_.nodes(&graphNode)) {
                    if (latticeNode.from() != bos &&
                        latticeNode.score() > min &&
                        latticeNode.score() + d->ime_->maxDistance() > max &&
                        !static_cast<const PinyinLatticeNode &>(latticeNode)
                             .anyCorrectionOnPath()) {
                        d->candidates_.push_back(
                            latticeNode.toSentenceResult(adjust));
                    }
                }
            }
        }
        std::sort(d->candidates_.begin() + beginSize, d->candidates_.end(),
                  std::greater<>());
        {
            size_t index = 0;
            size_t count = 0;
            const auto limit = d->ime_->wordCandidateLimit();
            std::unordered_set<CandidateDedupKey, CandidateDedupKeyHash>
                duplicateCandidates;
            auto &candidatesSet = d->candidatesSet_;
            candidatesSet.clear();
            std::erase_if(d->candidates_,
                          [&candidatesSet, &duplicateCandidates, &index, &count,
                           beginSize, limit](const SentenceResult &candidate) {
                              bool beforeBeginSize = index++ < beginSize;
                              auto key = candidateDedupKey(candidate);
                              if (duplicateCandidates.contains(key)) {
                                  return true;
                              }

                              if (!beforeBeginSize && limit) {
                                  const bool isSinglePinyinWord =
                                      candidate.sentence().size() == 1 &&
                                      candidate.sentence()
                                              .front()
                                              ->as<PinyinLatticeNode>()
                                              .encodedPinyin()
                                              .size() == 2;
                                  if (!isSinglePinyinWord) {
                                      if (count >= limit) {
                                          return true;
                                      }
                                      count++;
                                  }
                              }

                              candidatesSet.insert(key.text_);
                              duplicateCandidates.insert(std::move(key));
                              return false;
                          });
        }

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
    c = std::max(c, len);

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
                auto from = (*iter)->index();
                auto to = (*std::next(iter))->index();
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
                        node->as<PinyinLatticeNode>().encodedPinyin();
                    auto nthPinyin = std::distance(node->path().begin(), iter);
                    PinyinInitial bestInitial = syls[0].first;
                    PinyinFinal bestFinal = syls[0].second[0].first;

                    // Try to match the candidate syllables from all possible
                    // none-fuzzy possible syls.
                    if (static_cast<size_t>((nthPinyin * 2) + 2) <=
                        candidatePinyin.size()) {
                        auto candidateInitial = static_cast<PinyinInitial>(
                            candidatePinyin[nthPinyin * 2]);
                        auto candidateFinal = static_cast<PinyinFinal>(
                            candidatePinyin[(nthPinyin * 2) + 1]);

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
            if (item.type_ != SelectedPinyinType::Separator) {
                newSentence.push_back(item.word_.word());
            }
        }
    }
    return newSentence;
}

std::vector<HistoryBigram::WordWithCode>
PinyinContext::selectedWordsWithPinyin() const {
    FCITX_D();
    std::vector<HistoryBigram::WordWithCode> newSentence;
    for (const auto &s : d->selected_) {
        for (const auto &item : s) {
            if (item.type_ != SelectedPinyinType::Separator) {
                newSentence.emplace_back(item.word_.word(),
                                         item.encodedPinyin());
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
            if (!item.encodedPinyin().empty()) {
                if (!pinyin.empty()) {
                    pinyin.push_back('\'');
                }
                pinyin += PinyinEncoder::decodeFullPinyin(item.encodedPinyin());
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
        if (!node->as<PinyinLatticeNode>().encodedPinyin().empty()) {
            if (!pinyin.empty()) {
                pinyin.push_back('\'');
            }
            pinyin += PinyinEncoder::decodeFullPinyin(
                node->as<PinyinLatticeNode>().encodedPinyin());
        }
    }
    return pinyin;
}

void PinyinContext::learn() {
    FCITX_D();
    if (!selected()) {
        return;
    }

    std::vector<HistoryBigram::WordWithCode> newSentence;
    if (auto [result, encodedWordPinyin] = d->learnWord();
        result != LearnWordResult::Ignored) {
        // Do not insert custom to history for the first time.
        if (result == LearnWordResult::Normal) {
            // Create new sentence with the whole new learned word.
            newSentence.push_back({sentence(), encodedWordPinyin});
        } else {
            return;
        }
    } else {
        newSentence = selectedWordsWithPinyin();
    }

    if (std::ranges::any_of(newSentence, [](const auto &word) {
            return word.second.empty();
        })) {
        // Don't add to history if there is any non-pinyin word.
        return;
    }

    auto context = contextWordsWithPinyin();
    d->ime_->model()->history().addWithContext(contextWordsWithPinyin(),
                                               std::move(newSentence));
}

void PinyinContext::setContextWords(
    const std::vector<std::string> &contextWords) {
    FCITX_D();
    d->contextWords_.clear();
    appendContextWords(contextWords);
}

void PinyinContext::clearContextWords() {
    FCITX_D();
    d->contextWords_.clear();
}

void PinyinContext::appendContextWords(
    const std::vector<std::string> &contextWords) {
    FCITX_D();

    size_t needed = LanguageModel::maxOrder() - 1;

    for (const auto &word :
         std::span{contextWords}.last(std::min(contextWords.size(), needed))) {
        d->contextWords_.push_back(
            PinyinWordNode({word, ""}, d->ime_->model()->index(word)));
    }
    while (d->contextWords_.size() > needed) {
        d->contextWords_.pop_front();
    }
}

std::vector<std::string> PinyinContext::contextWords() const {
    FCITX_D();
    std::vector<std::string> words;
    words.reserve(d->contextWords_.size());
    for (const auto &word : d->contextWords_) {
        words.push_back(word.word());
    }
    return words;
}

void PinyinContext::setContextWordsWithPinyin(
    const std::vector<HistoryBigram::WordWithCode> &contextWordsWithPinyin) {
    FCITX_D();
    d->contextWords_.clear();
    appendContextWordsWithPinyin(contextWordsWithPinyin);
}

void PinyinContext::appendContextWordsWithPinyin(
    const std::vector<HistoryBigram::WordWithCode> &contextWordsWithPinyin) {
    FCITX_D();

    size_t needed = LanguageModel::maxOrder() - 1;

    for (const auto &word : std::span{contextWordsWithPinyin}.last(
             std::min(contextWordsWithPinyin.size(), needed))) {
        d->contextWords_.push_back(
            PinyinWordNode(word, d->ime_->model()->index(word.first)));
    }
    while (d->contextWords_.size() > needed) {
        d->contextWords_.pop_front();
    }
}

std::vector<HistoryBigram::WordWithCode>
PinyinContext::contextWordsWithPinyin() const {
    FCITX_D();
    std::vector<HistoryBigram::WordWithCode> words;
    words.reserve(d->contextWords_.size());
    for (const auto &word : d->contextWords_) {
        words.push_back({word.word(), word.encodedPinyin()});
    }
    return words;
}

bool PinyinContext::learnWord() { return false; }

PinyinIME *PinyinContext::ime() const {
    FCITX_D();
    return d->ime_;
}
} // namespace libime
