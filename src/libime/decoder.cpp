/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#include <boost/ptr_container/ptr_vector.hpp>
#include <limits>
#include <memory>
#include <queue>
#include <sstream>

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
    DecoderPrivate(Decoder *q, Dictionary *dict, LanguageModel *model)
        : q_ptr(q), dict_(dict), model_(model) {}

    LatticeMap buildLattice(State state, const SegmentGraph &graph) const {
        FCITX_Q();
        LatticeMap lattice;

        lattice[&graph.start()].push_back(
            q->createLatticeNode(model_, "", model_->beginSentence(), nullptr,
                                 &graph.start(), 0, state));
        dict_->matchPrefix(
            graph, [this, &graph,
                    &lattice](const std::vector<const SegmentGraphNode *> &path,
                              boost::string_view entry, float adjust) {
                FCITX_Q();
                WordIndex idx;
                if (entry.empty()) {
                    idx = model_->endSentence();
                } else {
                    idx = model_->index(entry);
                }
                assert(path.front());
                auto node = q->createLatticeNode(
                    model_, entry, idx, path.front(), path.back(), adjust);
                if (node) {
                    lattice[path.back()].push_back(node);
                }
            });
        assert(lattice.count(&graph.end()));
        lattice[nullptr].push_back(q->createLatticeNode(
            model_, "", model_->endSentence(), &graph.end(), nullptr));
        return lattice;
    }

    Decoder *q_ptr;
    FCITX_DECLARE_PUBLIC(Decoder);
    Dictionary *dict_;
    LanguageModel *model_;
    UnknownHandler unknownHandler_;
};

Decoder::Decoder(Dictionary *dict, LanguageModel *model)
    : d_ptr(std::make_unique<DecoderPrivate>(this, dict, model)) {}

Decoder::~Decoder() {}

std::string concatNBest(NBestNode *node, boost::string_view sep = "") {
    std::stringstream ss;
    while (node != nullptr) {
        ss << node->node_->word() << sep;
        node = node->next_;
    }
    return ss.str();
}

Lattice Decoder::decode(const SegmentGraph &graph, size_t nbest, float max,
                        float min, State beginState) const {
    FCITX_D();
    if (beginState.empty()) {
        beginState = d->model_->beginState();
    }
    auto lattice = d->buildLattice(beginState, graph);
    // std::cout << "Lattice Size: " << lattice.size() << std::endl;
    // avoid repeated allocation
    State state = d->model_->nullState();

    // forward search
    auto updateForNode = [&](const SegmentGraphNode *graphNode) {
        if (!lattice.count(graphNode)) {
            return;
        }
        auto &latticeNodes = lattice[graphNode];
        std::unordered_map<const SegmentGraphNode *,
                           std::tuple<float, LatticeNode *, State>>
            unknownIdCache;
        for (auto &node : latticeNodes) {
            auto from = node.from();
            float maxScore = -std::numeric_limits<float>::max();
            LatticeNode *maxNode = nullptr;
            State maxState = d->model_->nullState();
            if (node.idx() == d->model_->unknown()) {
                auto iter = unknownIdCache.find(from);
                if (iter != unknownIdCache.end()) {
                    std::tie(maxScore, maxNode, maxState) = iter->second;
                }
            }

            if (!maxNode) {
                for (auto &parent : lattice[from]) {
                    auto score =
                        parent.score() +
                        d->model_->score(parent.state(), node.idx(), state);
                    if (score > maxScore) {
                        maxScore = score;
                        maxNode = &parent;
                        maxState = state;
                    }
                }

                if (node.idx() == d->model_->unknown()) {
                    unknownIdCache.emplace(
                        std::piecewise_construct, std::forward_as_tuple(from),
                        std::forward_as_tuple(maxScore, maxNode, maxState));
                }
            }

            assert(maxNode);
            node.setScore(maxScore + node.cost());
            node.setPrev(maxNode);
            node.state() = std::move(maxState);
#if 0
            {
                auto pos = &node;
                auto bos = &lattice[&graph.start()][0];
                std::string sentence;
                while (pos != bos) {
                    sentence = pos->word_ + " " + sentence;
                    pos = pos->prev_;
                }
                std::cout << &node << " " << graphNode << " " << sentence << " " << node.score_ << std::endl;
            }
#endif
        }
    };
    for (size_t i = 1; i <= graph.size(); i++) {
        for (auto &graphNode : graph.nodes(i)) {
            updateForNode(&graphNode);
        }
    }
    updateForNode(nullptr);

    auto p = std::make_unique<LatticePrivate>();
    // backward search
    if (nbest == 1) {
        assert(lattice[&graph.start()].size() == 1);
        assert(lattice[nullptr].size() == 1);
        auto pos = &lattice[nullptr][0];
        p->nbests.push_back(pos->toSentenceResult());
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
                    auto score = d->model_->score(from.state(),
                                                  node->node_->idx(), state) +
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
                result.emplace_back(pivot->node_->to(), pivot->node_->word());
                pivot = pivot->next_;
            }
            p->nbests.emplace_back(std::move(result), node->fn_);
        }
    }

    p->lattice = std::move(lattice);
    return {p.release()};
}

LatticeNode *Decoder::createLatticeNode(LanguageModel *model,
                                        boost::string_view word, WordIndex idx,
                                        const SegmentGraphNode *from,
                                        const SegmentGraphNode *to, float cost,
                                        State state) const {
    return createLatticeNodeImpl(model, word, idx, from, to, cost, state);
}

LatticeNode *Decoder::createLatticeNodeImpl(LanguageModel *model,
                                            boost::string_view word,
                                            WordIndex idx,
                                            const SegmentGraphNode *from,
                                            const SegmentGraphNode *to,
                                            float cost, State state) const {
    return new LatticeNode(model, word, idx, from, to, cost, state);
}
}
