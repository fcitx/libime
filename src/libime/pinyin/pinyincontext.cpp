/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "pinyincontext.h"
#include <algorithm>
#include <any>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <optional>
#include <ranges>
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

// Get encoded full pinyin.
std::string sentenceEncodedFullPinyin(const SentenceResult &candidate) {
    std::string pinyin;
    size_t length = 0;
    for (const auto &node : candidate.sentence()) {
        if (!node->as<PinyinLatticeNode>().encodedPinyin().empty()) {
            length += node->as<PinyinLatticeNode>().encodedPinyin().size();
        }
    }
    pinyin.reserve(length);
    for (const auto &node : candidate.sentence()) {
        if (!node->as<PinyinLatticeNode>().encodedPinyin().empty()) {
            pinyin += node->as<PinyinLatticeNode>().encodedPinyin();
        }
    }
    return pinyin;
}

size_t candidateSelectTo(const SentenceResult &candidate) {
    return candidate.sentence().empty()
               ? 0
               : candidate.sentence().back()->to()->index();
}

struct SelectedPinyin {
    SelectedPinyin(size_t s, PinyinWordNode word, SelectedPinyinType type)
        : offset_(s), word_(std::move(word)), type_(type) {}

    const std::string &encodedPinyin() const { return word_.encodedPinyin(); }

    size_t offset_;
    PinyinWordNode word_;
    SelectedPinyinType type_;
};

struct CandidateDedupByPinyin {
    std::string text_;
    std::string fullPinyin_;

    bool operator==(const CandidateDedupByPinyin &other) const {
        return text_ == other.text_ && fullPinyin_ == other.fullPinyin_;
    }
};

struct CandidateDedupByPinyinHash {
    size_t operator()(const CandidateDedupByPinyin &key) const {
        size_t seed = std::hash<std::string>()(key.text_);
        boost::hash_combine(seed, std::hash<std::string>()(key.fullPinyin_));
        return seed;
    }
};

CandidateDedupByPinyin candidateDedupByPinyin(const SentenceResult &candidate) {
    return {.text_ = candidate.toString(),
            .fullPinyin_ = sentenceEncodedFullPinyin(candidate)};
}

struct CandidateDedupBySelectRange {
    std::string text_;
    std::string end_;

    bool operator==(const CandidateDedupBySelectRange &other) const {
        return text_ == other.text_ && end_ == other.end_;
    }
};

struct CandidateDedupBySelectRangeHash {
    size_t operator()(const CandidateDedupBySelectRange &key) const {
        size_t seed = std::hash<std::string>()(key.text_);
        boost::hash_combine(seed, std::hash<std::string>()(key.end_));
        return seed;
    }
};

CandidateDedupBySelectRange
candidateDedupBySelectRange(const SentenceResult &candidate) {
    return {.text_ = candidate.toString(),
            .end_ = std::to_string(candidateSelectTo(candidate))};
}

bool hasCorrection(const SentenceResult &candidate) {
    return std::ranges::any_of(
        candidate.sentence(), [](const LatticeNode *node) {
            return node->as<PinyinLatticeNode>().isCorrection();
        });
}

// Input: 10, [(2,4), (4,5), (8,9)]
// Output: [(0,2), (5,8), (9,10)]
template <std::ranges::random_access_range R>
auto makeComplement(size_t size, R excluded) {
    const size_t count = std::ranges::size(excluded);
    return std::views::iota(size_t{0}, count + 1) |
           std::views::transform(
               [size, count, excluded = std::move(excluded)](size_t i) {
                   const size_t begin = i == 0 ? 0 : excluded[i - 1].second;
                   const size_t end = i == count ? size : excluded[i].first;
                   return std::pair{begin, end};
               }) |
           std::views::filter([](auto gap) { return gap.first != gap.second; });
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

    std::vector<PinyinSegmentFix> fixes_;

    SegmentGraph buildGraph() const {
        FCITX_Q();
        const size_t start = q->selectedLength();
        const std::string_view input =
            std::string_view{q->userInput()}.substr(start);
        SegmentGraph graph{std::string{input}};
        const auto spProfile = matchState_.shuangpinProfile();

        // [] is a fix, a fixed span no need to parse
        //                  ↓
        // userInput: nihao[xi]an
        //              ↑      ↑
        // Others are free span, parse with parseUserInput
        const auto firstFix = std::ranges::lower_bound(
            fixes_, start, {}, &PinyinSegmentFix::start_);
        const auto relativeFixes =
            std::span{fixes_}.subspan(
                static_cast<size_t>(firstFix - fixes_.begin())) |
            std::views::transform([start](const PinyinSegmentFix &fix) {
                return PinyinSegmentFix{fix.start_ - start, fix.end_ - start,
                                        fix.fixed_};
            });
        auto fixedSpans =
            relativeFixes |
            std::views::transform([start](const PinyinSegmentFix &fix) {
                return std::pair{fix.start_, fix.end_};
            });

        for (const auto &[from, to, fixed] : relativeFixes) {
            graph.ensureNode(from).setAnnotation(fixed);
            graph.addNext(from, to);
        }
        for (const auto &[from, to] :
             makeComplement(input.size(), fixedSpans)) {
            const std::string freeSpan{input.substr(from, to - from)};
            const SegmentGraph subGraph =
                spProfile ? PinyinEncoder::parseUserShuangpin(
                                freeSpan, *spProfile, ime_->fuzzyFlags())
                          : PinyinEncoder::parseUserPinyin(
                                freeSpan, ime_->correctionProfile().get(),
                                ime_->fuzzyFlags());
            for (size_t i = 0, e = subGraph.size(); i <= e; i++) {
                for (const auto &node : subGraph.nodes(i)) {
                    for (const auto &next : node.nexts()) {
                        graph.addNext(from + node.index(), from + next.index());
                    }
                }
            }
        }
        return graph;
    }

    // Discard the decode state attached to the graph nodes after the
    // absolute input position pos, keeping everything up to and including
    // the node at pos. Only the node keyed state is dropped; the graph
    // itself and the LRU caches in matchState_ are kept. Callers must run
    // update() afterwards.
    void discardDecodeStateAfter(size_t pos) {
        FCITX_Q();
        const size_t start = q->selectedLength();
        size_t begin = 0;
        if (pos >= start) {
            begin = pos - start + 1;
        }
        std::unordered_set<const SegmentGraphNode *> nodes;
        for (size_t i = begin; i <= segs_.size(); i++) {
            for (const auto &node : segs_.nodes(i)) {
                nodes.insert(&node);
            }
        }
        if (nodes.empty()) {
            return;
        }
        lattice_.discardNode(nodes);
        matchState_.discardNode(nodes);
    }

    // Remove the fixes that are affected by an edit starting at from.
    // Return the start of the first removed fix, if any. If any fix is
    // removed, the decode state after that point is discarded as well,
    // callers must run update() afterwards.
    std::optional<size_t> discardFixesAfter(size_t from) {
        // The fixes are ordered and non-overlapping, so their end_ is
        // strictly increasing: the removed fixes always form a suffix.
        auto iter =
            std::ranges::upper_bound(fixes_, from, {}, &PinyinSegmentFix::end_);
        if (iter == fixes_.end()) {
            return std::nullopt;
        }
        const size_t first = iter->start_;
        fixes_.erase(iter, fixes_.end());
        discardDecodeStateAfter(first);
        return first;
    }

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

        std::unordered_map<CandidateDedupByPinyin, size_t,
                           CandidateDedupByPinyinHash>
            duplicateByPinyin;
        auto insertCandidate = [this,
                                &duplicateByPinyin](SentenceResult candidate) {
            auto byPinyin = candidateDedupByPinyin(candidate);
            auto byPinyinIter = duplicateByPinyin.find(byPinyin);
            if (byPinyinIter != duplicateByPinyin.end()) {
                auto &oldCandidate = candidatesToCursor_[byPinyinIter->second];
                if (std::make_tuple(candidateSelectTo(candidate),
                                    candidate.score()) >
                    std::make_tuple(candidateSelectTo(oldCandidate),
                                    oldCandidate.score())) {
                    oldCandidate = std::move(candidate);
                }
                return;
            }

            candidatesToCursor_.push_back(std::move(candidate));
            duplicateByPinyin.emplace(byPinyin, candidatesToCursor_.size() - 1);
            candidatesToCursorSet_.insert(std::move(byPinyin.text_));
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
                if (candidateSelectTo(candidate) + start > currentCursor) {
                    continue;
                }
                insertCandidate(candidate);
            } else if (sentence.size() > 1) {
                auto newSentence = sentence;
                while (!newSentence.empty() &&
                       candidateSelectTo(candidate) + start > currentCursor) {
                    newSentence.pop_back();
                }
                if (!newSentence.empty()) {
                    SentenceResult partial(newSentence,
                                           newSentence.back()->score());
                    insertCandidate(std::move(partial));
                }
            }
        }

        std::unordered_set<CandidateDedupBySelectRange,
                           CandidateDedupBySelectRangeHash>
            duplicateByRange;
        std::erase_if(
            candidatesToCursor_,
            [&duplicateByRange](const SentenceResult &candidate) {
                if (duplicateByRange
                        .insert(candidateDedupBySelectRange(candidate))
                        .second) {
                    return false;
                }
                return hasCorrection(candidate);
            });
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
    const size_t pos = cursor();
    const bool cancelled = cancelTill(pos);
    const bool typed = InputBuffer::typeImpl(s, length);
    if (typed) {
        d->discardFixesAfter(pos);
    }
    if (cancelled || typed) {
        update();
    }
    return cancelled || typed;
}

void PinyinContext::erase(size_t from, size_t to) {
    FCITX_D();
    if (from == to) {
        return;
    }

    // check if erase everything
    if (from == 0 && to >= size()) {
        d->clearCandidates();
        d->selected_.clear();
        d->lattice_.clear();
        d->matchState_.clear();
        d->segs_ = SegmentGraph();
        d->fixes_.clear();
    } else {
        cancelTill(from);
        d->discardFixesAfter(from);
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
    const auto touchedFix = d->discardFixesAfter(pos);
    InputBuffer::setCursor(pos);
    if (cancelled || touchedFix) {
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

void PinyinContext::setPinyinFixes(std::vector<PinyinSegmentFix> fixes) {
    FCITX_D();
    if (d->fixes_ == fixes) {
        return;
    }

    for (size_t i = 0; i < fixes.size(); ++i) {
        const auto &fix = fixes[i];
        if (fix.start_ > fix.end_ || (i && (fixes[i - 1].start_ > fix.start_ ||
                                            fixes[i - 1].end_ > fix.start_))) {
            throw std::invalid_argument("Invalid pinyin fixes");
        }
    }

    const size_t start = selectedLength();
    auto it1 = std::ranges::lower_bound(d->fixes_, start, {},
                                        &PinyinSegmentFix::start_);
    auto it2 =
        std::ranges::lower_bound(fixes, start, {}, &PinyinSegmentFix::start_);
    // Both sequences are strictly ordered and non-overlapping, the first
    // mismatched position between the two sequences inherently corresponds to
    // the earliest `start_` offset across all added, removed or modified
    // fixes.
    auto [m1, m2] =
        std::ranges::mismatch(it1, d->fixes_.end(), it2, fixes.end());
    size_t firstChanged = std::numeric_limits<size_t>::max();
    if (m1 != d->fixes_.end()) {
        firstChanged = std::min(firstChanged, m1->start_);
    }
    if (m2 != fixes.end()) {
        firstChanged = std::min(firstChanged, m2->start_);
    }
    d->fixes_ = std::move(fixes);
    if (firstChanged != std::numeric_limits<size_t>::max()) {
        d->discardDecodeStateAfter(firstChanged);
    }
    update();
}

const std::vector<PinyinSegmentFix> &PinyinContext::pinyinFixes() const {
    FCITX_D();
    return d->fixes_;
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
        State state = this->state();
        SegmentGraph newGraph = d->buildGraph();
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
            std::unordered_set<CandidateDedupByPinyin,
                               CandidateDedupByPinyinHash>
                duplicateByPinyin;
            std::unordered_set<CandidateDedupBySelectRange,
                               CandidateDedupBySelectRangeHash>
                duplicateByRange;
            auto &candidatesSet = d->candidatesSet_;
            candidatesSet.clear();
            std::erase_if(
                d->candidates_,
                [&candidatesSet, &duplicateByPinyin, &duplicateByRange, &index,
                 &count, beginSize, limit](const SentenceResult &candidate) {
                    bool beforeBeginSize = index++ < beginSize;
                    auto byPinyin = candidateDedupByPinyin(candidate);
                    if (duplicateByPinyin.contains(byPinyin)) {
                        return true;
                    }
                    auto byRange = candidateDedupBySelectRange(candidate);
                    if (duplicateByRange.contains(byRange) &&
                        hasCorrection(candidate)) {
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

                    candidatesSet.insert(byPinyin.text_);
                    duplicateByPinyin.insert(std::move(byPinyin));
                    duplicateByRange.insert(std::move(byRange));
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

std::tuple<std::string, size_t, std::vector<std::pair<size_t, size_t>>>
PinyinContext::preeditWithCursorHighlight() const {
    return preeditWithCursorHighlight(ime()->preeditMode());
}

std::string PinyinContext::preedit(PinyinPreeditMode mode) const {
    return preeditWithCursor(mode).first;
}

std::pair<std::string, size_t>
PinyinContext::preeditWithCursor(PinyinPreeditMode mode) const {
    auto [text, cursor, highlights] = preeditWithCursorHighlight(mode);
    (void)highlights;
    return {std::move(text), cursor};
}

std::tuple<std::string, size_t, std::vector<std::pair<size_t, size_t>>>
PinyinContext::preeditWithCursorHighlight(PinyinPreeditMode mode) const {
    return preeditWithCursorHighlightImpl(mode, cursor());
}

std::tuple<std::string, size_t, std::vector<std::pair<size_t, size_t>>>
PinyinContext::preeditWithCursorHighlightImpl(PinyinPreeditMode mode,
                                              size_t inputCursor) const {
    FCITX_D();
    std::vector<std::pair<size_t, size_t>> highlights;
    std::string ss = selectedSentence();
    const auto len = selectedLength();
    auto c = inputCursor;
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
                const size_t previousEnd = resultSize;
                if (!first) {
                    ss += " ";
                    resultSize += 1;
                } else {
                    first = false;
                }
                const auto from = (*iter)->index();
                const auto to = (*std::next(iter))->index();
                const size_t startPivot = resultSize;
                const auto pinyin = d->segs_.segment(from, to);
                const auto *fixed =
                    std::any_cast<PinyinSyllable>(&(*iter)->annotation());
                MatchedPinyinSyllables syls;
                std::string actualPinyin;
                if (fixed) {
                    // A fixed syllable is displayed as the fixed pinyin, not
                    // as the raw input.
                    syls = {{fixed->initial(), {{fixed->final(), false}}}};
                    actualPinyin = PinyinEncoder::initialFinalToPinyinString(
                        fixed->initial(), fixed->final());
                } else if (mode == PinyinPreeditMode::Pinyin) {
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

                size_t cursorInPinyin =
                    c > from + len && c <= to + len ? c - from - len : 0;
                const size_t realCursorInPinyin =
                    cursor() > from + len && cursor() <= to + len
                        ? cursor() - from - len
                        : 0;
                if (!actualPinyin.empty()) {
                    // only modify actualPinyin when realCursorInPinyin
                    if (realCursorInPinyin && useShuangpin() &&
                        pinyin.size() == 2 && realCursorInPinyin == 1 &&
                        syls[0].first == PinyinInitial::Zero) {
                        // only add _ to real cursor
                        actualPinyin =
                            fcitx::stringutils::concat("_", actualPinyin);
                    }
                    // only move cursor when cursorInPinyin
                    if (cursorInPinyin) {
                        if (useShuangpin()) {
                            // when useShuangpin, pinyin.size() can only be
                            // 1 or 2
                            if (pinyin.size() == 2 && cursorInPinyin == 1) {
                                if (syls[0].first == PinyinInitial::Zero) {
                                    if (!actualPinyin.starts_with('_')) {
                                        // |aa → |a
                                        // a|a → |a   ← below code is for this
                                        // aa| → a|
                                        cursorInPinyin = 0;
                                    }
                                } else {
                                    cursorInPinyin =
                                        PinyinEncoder::initialToString(
                                            syls[0].first)
                                            .size();
                                }
                            } else {
                                //  |0 → |012345
                                //  0| → 012345|   ← ensures these two rules
                                // 01| → 012345|   ←──┘
                                cursorInPinyin = actualPinyin.size();
                            }
                        } else {
                            // |012345 → |012
                            // 0|12345 → 0|12
                            // 01|2345 → 01|2
                            // 012|345 → 01|2
                            // 0123|45 → 01|2
                            // 01234|5 → 01|2
                            // 012345| → 012|
                            if (c == to + len) {
                                cursorInPinyin = actualPinyin.size();
                            } else {
                                cursorInPinyin = std::min(
                                    fcitx::utf8::length(actualPinyin) - 1,
                                    cursorInPinyin);
                                cursorInPinyin = fcitx::utf8::ncharByteLength(
                                    actualPinyin.begin(), cursorInPinyin);
                            }
                        }
                    }
                    ss.append(actualPinyin);
                    resultSize += actualPinyin.size();
                } else {
                    ss.append(pinyin.data(), pinyin.size());
                    resultSize += pinyin.size();
                }
                if (fixed) {
                    highlights.emplace_back(startPivot, resultSize);
                }
                if (c > from + len && c <= to + len) {
                    actualCursor = cursorInPinyin == 0
                                       ? previousEnd
                                       : startPivot + cursorInPinyin;
                }
            }
        }
    }
    if (c == size()) {
        actualCursor = resultSize;
    }
    return {std::move(ss), actualCursor, std::move(highlights)};
}

// A fix can render shorter than the raw input. With "012345" fixed to
// "012":
//
//   ┌─────────┬─────────┐
//   │ preedit │   raw   │
//   ├─────────┼─────────┤
//   │ |012    │ |012345 │
//   │ 0|12    │ 0|12345 │
//   │ 01|2    │ 01|2345 │
//   │ 012|    │ 012345| │
//   └─────────┴─────────┘
//
// A fix can also render longer than the raw input. With "012" fixed to
// "012345":
//
//   ┌─────────┬──────┐
//   │ preedit │ raw  │
//   ├─────────┼──────┤
//   │ |012345 │ |012 │
//   │ 0|12345 │ 0|12 │
//   │ 01|2345 │ 01|2 │
//   │ 012|345 │ 01|2 │
//   │ 0123|45 │ 01|2 │
//   │ 01234|5 │ 01|2 │
//   │ 012345| │ 012| │
//   └─────────┴──────┘
std::optional<size_t>
PinyinContext::cursorForPreeditPosition(size_t preeditByteCursor,
                                        PinyinPreeditMode mode) const {
    const auto selectedBytes = selectedSentence().size();
    // The selected sentence has no raw input cursor.
    if (preeditByteCursor < selectedBytes) {
        return std::nullopt;
    }

    // The preedit cursor only moves forward as the input cursor grows, so the
    // positions whose preedit cursor is before the clicked position form a
    // prefix. Find the first position that reaches the clicked position.
    const auto positions = std::views::iota(selectedLength(), size() + 1);
    const auto firstAfter =
        std::ranges::partition_point(positions, [&](size_t p) {
            return std::get<1>(preeditWithCursorHighlightImpl(mode, p)) <
                   preeditByteCursor;
        });
    // Every position is before the clicked position, i.e. it is past the end
    // of the preedit.
    if (firstAfter == positions.end()) {
        return size();
    }

    size_t target = *firstAfter;
    // The first position already moved past the clicked position, so step
    // back to the previous one.
    if (target > selectedLength() && std::get<1>(preeditWithCursorHighlightImpl(
                                         mode, target)) > preeditByteCursor) {
        --target;
    }

    auto targetCursor =
        std::get<1>(preeditWithCursorHighlightImpl(mode, target));

    // Among consecutive positions sharing the same preedit cursor, return the
    // leftmost one.
    // e.g. input "aa|aa" and "aaa|a" both render as predit "a| a",
    // if user clicks "a |a", we need map it to "aa|aa" not "aaa|a"
    while (target > selectedLength() &&
           std::get<1>(preeditWithCursorHighlightImpl(mode, target - 1)) ==
               targetCursor) {
        --target;
    }
    return target;
}

std::optional<size_t>
PinyinContext::cursorForPreeditPosition(size_t preeditByteCursor) const {
    return cursorForPreeditPosition(preeditByteCursor, ime()->preeditMode());
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
