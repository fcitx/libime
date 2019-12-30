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

#include "decoder.h"
#include "datrie.h"
#include "languagemodel.h"
#include "lattice_p.h"
#include "utils.h"
#include <boost/functional/hash.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include <chrono>
#include <limits>
#include <memory>
#include <queue>
#include <unordered_set>

namespace libime {

struct NBestNode {
    NBestNode(const LatticeNode *node) : node_(node) {}

    const LatticeNode *node_;
    // for nbest
    float gn_ = 0.0f;
    float fn_ = -std::numeric_limits<float>::max();
    NBestNode *next_ = nullptr;
};

class DecoderPrivate {
public:
    DecoderPrivate(const Dictionary *dict, const LanguageModelBase *model)
        : dict_(dict), model_(model) {}

    // Try to update lattice based on existing data.
    bool
    buildLattice(const Decoder *q, Lattice &l,
                 const std::unordered_set<const SegmentGraphNode *> &ignore,
                 const State &state, const SegmentGraph &graph,
                 size_t frameSize, void *helper) const;

    void
    forwardSearch(const Decoder *q, const SegmentGraph &graph, Lattice &lattice,
                  const std::unordered_set<const SegmentGraphNode *> &ignore,
                  size_t beamSize) const;
    void backwardSearch(const SegmentGraph &graph, Lattice &l, size_t nbest,
                        float max, float min) const;

    const Dictionary *dict_;
    const LanguageModelBase *model_;
};

bool DecoderPrivate::buildLattice(
    const Decoder *q, Lattice &l,
    const std::unordered_set<const SegmentGraphNode *> &ignore,
    const State &state, const SegmentGraph &graph, size_t frameSize,
    void *helper) const {
    LatticeMap &lattice = l.d_ptr->lattice_;

    // Create the root node.
    if (!lattice.count(&graph.start())) {
        lattice[&graph.start()].push_back(
            q->createLatticeNode(graph, model_, "", model_->beginSentence(),
                                 {nullptr, &graph.start()}, state, 0));
    }

    std::unordered_map<
        std::pair<const SegmentGraphNode *, const SegmentGraphNode *>, size_t,
        boost::hash<
            std::pair<const SegmentGraphNode *, const SegmentGraphNode *>>>
        dupPath;

    auto dictMatchCallback = [this, &graph, &lattice, &dupPath, q, frameSize](
                                 const SegmentGraphPath &path, WordNode &word,
                                 float adjust,
                                 std::unique_ptr<LatticeNodeData> data) {
        if (InvalidWordIndex == word.idx()) {
            auto idx = model_->index(word.word());
            word.setIdx(idx);
        }
        assert(path.front());
        size_t &dupSize = dupPath[std::make_pair(path.front(), path.back())];
        if ((frameSize && (dupSize > frameSize)) &&
            path.front() != &graph.start()) {
            return;
        }
        auto node = q->createLatticeNode(graph, model_, word.word(), word.idx(),
                                         path, model_->nullState(), adjust,
                                         std::move(data), dupSize == 0);
        if (node) {
            lattice[path.back()].push_back(node);
            dupSize++;
        }
    };

    dict_->matchPrefix(graph, dictMatchCallback, ignore, helper);
    if (!lattice.count(&graph.end())) {
        return false;
    }

    // Create the node for end.
    lattice[nullptr].push_back(
        q->createLatticeNode(graph, model_, "", model_->endSentence(),
                             {&graph.end(), nullptr}, model_->nullState()));
    return true;
}

void DecoderPrivate::forwardSearch(
    const Decoder *q, const SegmentGraph &graph, Lattice &l,
    const std::unordered_set<const SegmentGraphNode *> &ignore,
    size_t beamSize) const {
    State state;
    LatticeMap &lattice = l.d_ptr->lattice_;
    std::unordered_map<const SegmentGraphNode *,
                       std::tuple<float, LatticeNode *, State>>
        unknownIdCache;
    auto start = &graph.start();
    // forward search
    auto updateForNode = [&](const SegmentGraphBase &,
                             const SegmentGraphNode *graphNode) {
        if (graphNode == start || !lattice.count(graphNode) ||
            ignore.count(graphNode)) {
            return true;
        }
        auto &latticeNodes = lattice[graphNode];
        for (auto &node : latticeNodes) {
            auto from = node.from();
            assert(graph.checkNodeInGraph(from));
            float maxScore = -std::numeric_limits<float>::max();
            LatticeNode *maxNode = nullptr;
            State maxState;
            bool isUnknown = model_->isNodeUnknown(node);
            if (isUnknown) {
                auto iter = unknownIdCache.find(from);
                if (iter != unknownIdCache.end()) {
                    std::tie(maxScore, maxNode, maxState) = iter->second;
                }
            }

            if (!maxNode) {
                auto iter = lattice.find(from);
                // assert(iter != lattice.end());
                if (iter == lattice.end()) {
                    continue;
                }
                auto &searchFrom = iter->second;
                auto searchSize = beamSize;
                if (searchSize) {
                    searchSize = std::min(searchSize, lattice[from].size());
                } else {
                    searchSize = lattice[from].size();
                }
                for (auto &parent :
                     searchFrom | boost::adaptors::sliced(0, searchSize)) {
                    auto score = parent.score() +
                                 model_->score(parent.state(), node, state);
                    if (score > maxScore) {
                        maxScore = score;
                        maxNode = &parent;
                        maxState = state;
                    }
                }

                if (isUnknown) {
                    unknownIdCache.emplace(
                        std::piecewise_construct, std::forward_as_tuple(from),
                        std::forward_as_tuple(maxScore, maxNode, maxState));
                }
            }

            assert(maxNode);
            node.setScore(maxScore + node.cost());
            node.setPrev(maxNode);
            node.state() = maxState;
        }
        if (q->needSort(graph, graphNode)) {
            latticeNodes.sort(
                [](const LatticeNode &lhs, const LatticeNode &rhs) {
                    return lhs.score() > rhs.score();
                });
        }
        return true;
    };

    graph.bfs(start, updateForNode);
    updateForNode(graph, nullptr);
}

std::string concatNBest(NBestNode *node, std::string_view sep = "") {
    std::string result;
    while (node != nullptr) {
        result.append(node->node_->word());
        result.append(sep.data(), sep.size());
        node = node->next_;
    }
    return result;
}

void DecoderPrivate::backwardSearch(const SegmentGraph &graph, Lattice &l,
                                    size_t nbest, float max, float min) const {
    auto &lattice = l.d_ptr->lattice_;
    State state;
    // backward search
    if (nbest == 1) {
        assert(lattice[&graph.start()].size() == 1);
        assert(lattice[nullptr].size() == 1);
        auto pos = &lattice[nullptr][0];
        l.d_ptr->nbests_.push_back(pos->toSentenceResult());
    } else {
        struct NBestNodeLess {
            bool operator()(const NBestNode *lhs, const NBestNode *rhs) const {
                return lhs->fn_ < rhs->fn_;
            }
        };
        std::priority_queue<NBestNode *, std::vector<NBestNode *>,
                            NBestNodeLess>
            q, result;
        std::unordered_set<std::string> dup;
        std::list<NBestNode> nbestNodePool;

        auto eos = &lattice[nullptr][0];
        auto newNBestNode = [&nbestNodePool](const LatticeNode *node) {
            nbestNodePool.emplace_back(node);
            return &nbestNodePool.back();
        };
        q.push(newNBestNode(eos));
        auto bos = &lattice[&graph.start()][0];
        while (!q.empty()) {
            auto node = q.top();
            q.pop();
            if (bos == node->node_) {
                auto sentence = concatNBest(node);
                if (dup.count(sentence)) {
                    continue;
                }

                if (eos->score() - node->fn_ > max) {
                    break;
                }
                result.push(node);
                if (result.size() >= nbest) {
                    break;
                }
                dup.insert(sentence);
            } else {
                for (auto &from : lattice[node->node_->from()]) {
                    auto score =
                        model_->score(from.state(), *node->node_, state) +
                        node->node_->cost();
                    if (&from != bos && score < min)
                        continue;
                    auto parent = newNBestNode(&from);
                    parent->gn_ = score + node->gn_;
                    parent->fn_ = parent->gn_ + parent->node_->score();
                    parent->next_ = node;

                    q.push(parent);
                }
            }
        }

        while (!result.empty()) {
            auto node = result.top();
            result.pop();
            // loop twice to avoid problem
            size_t count = 0;
            // skip bos
            auto pivot = node->next_;
            while (pivot != nullptr) {
                pivot = pivot->next_;
                count++;
            }
            SentenceResult::Sentence result;
            result.reserve(count);
            pivot = node->next_;
            while (pivot != nullptr) {
                if (pivot->node_->to()) {
                    result.emplace_back(pivot->node_);
                }
                pivot = pivot->next_;
            }
            l.d_ptr->nbests_.emplace_back(std::move(result), node->fn_);
        }
    }
}

Decoder::Decoder(const Dictionary *dict, const LanguageModelBase *model)
    : d_ptr(std::make_unique<DecoderPrivate>(dict, model)) {}

Decoder::~Decoder() {}

const Dictionary *Decoder::dict() const {
    FCITX_D();
    return d->dict_;
}

const LanguageModelBase *Decoder::model() const {
    FCITX_D();
    return d->model_;
}

bool Decoder::decode(Lattice &l, const SegmentGraph &graph, size_t nbest,
                     const State &beginState, float max, float min,
                     size_t beamSize, size_t frameSize, void *helper) const {
    FCITX_D();
    LatticeMap &lattice = l.d_ptr->lattice_;
    // Clear the result.
    l.d_ptr->nbests_.clear();
    // Remove end node.
    lattice.erase(nullptr);
    std::unordered_set<const SegmentGraphNode *> ignore;
    // Add existing SegmentGraphNode to ignore set.
    for (auto &p : lattice) {
        ignore.insert(p.first);
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    if (!d->buildLattice(this, l, ignore, beginState, graph, frameSize,
                         helper)) {
        return false;
    }
    LIBIME_DEBUG() << "Build Lattice: " << millisecondsTill(t0);
    d->forwardSearch(this, graph, l, ignore, beamSize);
    LIBIME_DEBUG() << "Forward Search: " << millisecondsTill(t0);
    d->backwardSearch(graph, l, nbest, max, min);
    LIBIME_DEBUG() << "Backward Search: " << millisecondsTill(t0);
    return true;
}

LatticeNode *Decoder::createLatticeNodeImpl(
    const SegmentGraphBase &, const LanguageModelBase *, std::string_view word,
    WordIndex idx, SegmentGraphPath path, const State &state, float cost,
    std::unique_ptr<LatticeNodeData>, bool) const {
    return new LatticeNode(word, idx, std::move(path), state, cost);
}
} // namespace libime
