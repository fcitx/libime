/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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

constexpr int MAX_BACKWARD_SEARCH_SIZE = 10000;

struct NBestNode {
    NBestNode(const LatticeNode *node) : node_(node) {}

    const LatticeNode *node_;
    // for nbest
    float gn_ = 0.0f;
    float fn_ = -std::numeric_limits<float>::max();
    std::shared_ptr<NBestNode> next_;
};

template <typename T>
struct NBestNodeLess {
    bool operator()(const T &lhs, const T &rhs) const {
        return lhs->fn_ < rhs->fn_;
    }
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

    // std::vector is used here to make sure std::make_heap works.
    std::unordered_map<
        std::pair<const SegmentGraphNode *, const SegmentGraphNode *>,
        std::vector<std::unique_ptr<LatticeNode>>,
        boost::hash<
            std::pair<const SegmentGraphNode *, const SegmentGraphNode *>>>
        frames;

    auto dictMatchCallback = [this, &graph, &frames, q, frameSize](
                                 const SegmentGraphPath &path, WordNode &word,
                                 float adjust,
                                 std::unique_ptr<LatticeNodeData> data) {
        if (InvalidWordIndex == word.idx()) {
            auto idx = model_->index(word.word());
            word.setIdx(idx);
        }
        assert(path.front());
        auto &frame = frames[std::make_pair(path.front(), path.back())];
        const bool applyFrameSize =
            path.front() != &graph.start() && frameSize > 0;
        auto *node = q->createLatticeNode(
            graph, model_, word.word(), word.idx(), path, model_->nullState(),
            adjust, std::move(data), frame.empty());
        if (!node) {
            return;
        }

        frame.emplace_back(node);
        if (!applyFrameSize) {
            return;
        }
        // Make a maximum heap.
        auto scoreGreaterThan = [](const auto &lhs, const auto &rhs) {
            return lhs->score() > rhs->score();
        };
        // Just reach the limit, initialize the heap.
        if (frame.size() == frameSize) {
            for (auto &n : frame) {
                // Cache the score here.
                n->setScore(model_->singleWordScore(n->word()) + n->cost());
            }
            std::make_heap(frame.begin(), frame.end(), scoreGreaterThan);
        } else if (frame.size() == frameSize + 1) {
            // Cache the score here.
            node->setScore(model_->singleWordScore(node->word()) +
                           node->cost());
            // Take a short cut, check if node score greater than minimum
            if (scoreGreaterThan(node, frame[0])) {
                std::push_heap(frame.begin(), frame.end(), scoreGreaterThan);
                std::pop_heap(frame.begin(), frame.end(), scoreGreaterThan);
            }
            frame.pop_back();
        }
    };

    dict_->matchPrefix(graph, dictMatchCallback, ignore, helper);

    for (auto &[path, nodes] : frames) {
        auto &latticeUnit = lattice[path.second];
        for (auto &node : nodes) {
            latticeUnit.push_back(node.release());
        }
    }
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
    const auto *start = &graph.start();
    // forward search
    auto updateForNode = [&](const SegmentGraphBase &,
                             const SegmentGraphNode *graphNode) {
        if (graphNode == start || !lattice.count(graphNode) ||
            ignore.count(graphNode)) {
            return true;
        }
        auto &latticeNodes = lattice[graphNode];
        for (auto &node : latticeNodes) {
            const auto *from = node.from();
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
    while (node) {
        result.append(node->node_->word());
        result.append(sep.data(), sep.size());
        node = node->next_.get();
    }
    return result;
}

void DecoderPrivate::backwardSearch(const SegmentGraph &graph, Lattice &l,
                                    size_t nbest, float max, float min) const {
    auto &lattice = l.d_ptr->lattice_;
    State state;
    // backward search
    assert(lattice[&graph.start()].size() == 1);
    assert(lattice[nullptr].size() == 1);
    auto *pos = &lattice[nullptr][0];
    l.d_ptr->nbests_.push_back(pos->toSentenceResult());
    if (nbest > 1) {
        std::unordered_set<std::string> dup;
        dup.insert(l.d_ptr->nbests_[0].toString());
        using PriorityQueueType =
            std::priority_queue<std::shared_ptr<NBestNode>,
                                std::vector<std::shared_ptr<NBestNode>>,
                                NBestNodeLess<std::shared_ptr<NBestNode>>>;
        PriorityQueueType q, result;

        auto *eos = &lattice[nullptr][0];
        auto newNBestNode = [](const LatticeNode *node) {
            return std::make_shared<NBestNode>(node);
        };
        q.push(newNBestNode(eos));
        int acc = 0;
        auto *bos = &lattice[&graph.start()][0];
        while (!q.empty()) {
            std::shared_ptr<NBestNode> node = q.top();
            q.pop();
            if (bos == node->node_) {
                auto sentence = concatNBest(node.get());
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
                if (acc >= MAX_BACKWARD_SEARCH_SIZE) {
                    continue;
                }
                for (auto &from : lattice[node->node_->from()]) {
                    auto score =
                        model_->score(from.state(), *node->node_, state) +
                        node->node_->cost();
                    if (&from != bos && score < min) {
                        continue;
                    }
                    std::shared_ptr<NBestNode> parent = newNBestNode(&from);
                    parent->gn_ = score + node->gn_;
                    parent->fn_ = parent->gn_ + parent->node_->score();
                    parent->next_ = node;

                    if (eos->score() - node->gn_ <= max) {
                        q.push(std::move(parent));
                        acc++;
                        if (acc >= MAX_BACKWARD_SEARCH_SIZE) {
                            break;
                        }
                    }
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
            while (pivot) {
                pivot = pivot->next_;
                count++;
            }
            SentenceResult::Sentence result;
            result.reserve(count);
            pivot = node->next_;
            while (pivot) {
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
