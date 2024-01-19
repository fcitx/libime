/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_LATTICE_H_
#define _FCITX_LIBIME_CORE_LATTICE_H_

#include "libimecore_export.h"
#include <algorithm>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/type_erased.hpp>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/stringutils.h>
#include <libime/core/languagemodel.h>
#include <libime/core/segmentgraph.h>
#include <memory>
#include <type_traits>

namespace libime {

class Decoder;
class LatticePrivate;
class SegmentGraphNode;
class SegmentGraph;
class LatticeNode;

class SentenceResult {
public:
    using Sentence = std::vector<const LatticeNode *>;
    SentenceResult(Sentence sentence = {}, float score = 0.0F)
        : sentence_(std::move(sentence)), score_(score) {}
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(SentenceResult)

    const Sentence &sentence() const { return sentence_; }
    size_t size() const { return sentence_.size(); }

    float score() const { return score_; }
    void adjustScore(float adjust) { score_ += adjust; }
    void setScore(float score) { score_ = score; }

    bool operator<(const SentenceResult &rhs) const {
        return score_ < rhs.score_;
    }

    bool operator>(const SentenceResult &rhs) const {
        return score_ > rhs.score_;
    }

    std::string toString() const;

private:
    Sentence sentence_;
    float score_;
};

class LIBIMECORE_EXPORT WordNode {
public:
    WordNode(std::string_view word, WordIndex idx) : word_(word), idx_(idx) {}

    virtual ~WordNode() = default;
    WordNode(WordNode &&other) noexcept(
        std::is_nothrow_move_constructible<std::string>::value);
    WordNode &operator=(WordNode &&other) noexcept(
        std::is_nothrow_move_assignable<std::string>::value);

    const std::string &word() const { return word_; }
    WordIndex idx() const { return idx_; }
    void setIdx(WordIndex idx) { idx_ = idx; }

protected:
    std::string word_;
    WordIndex idx_;
};

class LIBIMECORE_EXPORT LatticeNodeData {
public:
    virtual ~LatticeNodeData() = default;
};

class LIBIMECORE_EXPORT LatticeNode : public WordNode {
public:
    LatticeNode(std::string_view word, WordIndex idx, SegmentGraphPath path,
                const State &state, float cost = 0)
        : WordNode(word, idx), path_(std::move(path)), cost_(cost),
          state_(state) {
        assert(path_.size() >= 2);
    }
    float cost() const { return cost_; }

    float score() const { return score_; }
    void setScore(float score) { score_ = score; }

    const SegmentGraphNode *from() const { return path_.front(); }
    const SegmentGraphNode *to() const { return path_.back(); }
    const SegmentGraphPath &path() const { return path_; }

    LatticeNode *prev() const { return prev_; }
    void setPrev(LatticeNode *prev) { prev_ = prev; }

    /// Return the full word till the begining of the sentence.
    std::string fullWord() const {
        size_t length = 0;
        const auto *pivot = this;
        while (pivot) {
            length += pivot->word().size();
            pivot = pivot->prev();
        }

        std::string result;
        result.resize(length);
        pivot = this;
        while (pivot) {
            const auto &word = pivot->word();
            length -= word.size();
            std::copy(word.begin(), word.end(), result.begin() + length);
            pivot = pivot->prev();
        }

        return result;
    }

    SentenceResult toSentenceResult(float adjust = 0.0F) const {
        SentenceResult::Sentence result;
        const auto *pivot = this;
        // to skip bos
        while (pivot->prev()) {
            if (pivot->to()) {
                result.emplace_back(pivot);
            }
            pivot = pivot->prev();
        }

        std::reverse(result.begin(), result.end());
        return {std::move(result), score() + adjust};
    }

    State &state() { return state_; }

protected:
    SegmentGraphPath path_;
    float cost_;
    float score_ = 0.0F;
    State state_;
    LatticeNode *prev_ = nullptr;
};

inline std::string SentenceResult::toString() const {
    return fcitx::stringutils::join(
        sentence_ | boost::adaptors::transformed(
                        [](const auto &item) -> const std::string & {
                            return item->word();
                        }),
        "");
}

// Lattice graph is a overlay graph on the SegmentGraph.
// Every node in SegmentGraph may have multiple corresponding LatticeNode.
// If there is an edge between two lattice nodes, there is a path between their
// corresponding SegmentGraphNode.
//
// For example, pinyin, "xian" has three SegmentGraphNodes.
// [0] ---- xian ----- [4]
//   \- xi -[2] - an -/
//
// while [0] has only one lattice node, the remaining nodes may has multiple
// nodes.
// So there is an extra "end" node that links every nodes from [4] to it.
class LIBIMECORE_EXPORT Lattice {
    friend class Decoder;
    friend class DecoderPrivate;
    friend class SegmentGraph;

public:
    Lattice();
    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(Lattice)

    size_t sentenceSize() const;
    const SentenceResult &sentence(size_t idx) const;
    void clear();
    void discardNode(const std::unordered_set<const SegmentGraphNode *> &node);

    using NodeRange =
        boost::any_range<LatticeNode, boost::bidirectional_traversal_tag,
                         const LatticeNode &>;

    NodeRange nodes(const SegmentGraphNode *node) const;

private:
    std::unique_ptr<LatticePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Lattice);
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_LATTICE_H_
