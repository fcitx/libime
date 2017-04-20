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
#include <boost/ptr_container/ptr_vector.hpp>
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <sstream>

namespace libime {

class LatticeNode {
public:
    LatticeNode(LanguageModel *model, boost::string_view word, WordIndex idx,
                const SegmentGraphNode *from, float cost = 0)
        : word_(word.to_string()), idx_(idx), from_(from), cost_(cost),
          state_(model->nullState()) {}

    std::string word_;
    WordIndex idx_;
    const SegmentGraphNode *from_;
    float cost_;
    float score_ = 0.0f;
    State state_;
    LatticeNode *prev_ = nullptr;
};

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
    DecoderPrivate(Dictionary *dict, LanguageModel *model)
        : dict_(dict), model_(model) {}

    std::unordered_map<const SegmentGraphNode *, std::vector<LatticeNode>>
    buildLattice(const SegmentGraph &graph) {
        std::unordered_map<const SegmentGraphNode *, std::vector<LatticeNode>>
            lattice;

        lattice[&graph.start()].emplace_back(model_, "",
                                             model_->beginSentence(), nullptr);
        lattice[&graph.start()][0].state_ = model_->beginState();
        dict_->matchPrefix(
            graph, [this, &graph,
                    &lattice](const std::vector<const SegmentGraphNode *> &path,
                              boost::string_view entry, float adjust) {
                WordIndex idx;
                if (entry.empty()) {
                    idx = model_->endSentence();
                } else {
                    idx = model_->index(entry);
                }
                if (idx == model_->unknown()) {
                    if (!unknownHandler_(graph, path, entry, adjust)) {
                        return;
                    }
                }
                assert(path.front());
                lattice[path.back()].emplace_back(model_, entry, idx,
                                                  path.front(), adjust);
            });
        assert(lattice.count(&graph.end()));
        lattice[nullptr].emplace_back(model_, "", model_->endSentence(),
                                      &graph.end());
        return lattice;
    }

    Dictionary *dict_;
    LanguageModel *model_;
    UnknownHandler unknownHandler_;
};

Decoder::Decoder(Dictionary *dict, LanguageModel *model)
    : d_ptr(std::make_unique<DecoderPrivate>(dict, model)) {}

Decoder::~Decoder() {}

std::string concatNBest(NBestNode *node, boost::string_view sep = "") {
    std::stringstream ss;
    while (node != nullptr) {
        ss << node->node_->word_ << sep;
        node = node->next_;
    }
    return ss.str();
}

void Decoder::decode(const SegmentGraph &graph, size_t nbest) {
    return decode(graph, nbest, std::numeric_limits<float>::max(),
                  -std::numeric_limits<float>::max());
}

void Decoder::decode(const SegmentGraph &graph, size_t nbest, float max,
                     float min) {
    FCITX_D();
    auto lattice = d->buildLattice(graph);
    // std::cout << "Lattice Size: " << lattice.size() << std::endl;

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
            auto from = node.from_;
            float max = -std::numeric_limits<float>::max();
            LatticeNode *maxNode = nullptr;
            State maxState = d->model_->nullState();
            if (node.idx_ == d->model_->unknown()) {
                auto iter = unknownIdCache.find(from);
                if (iter != unknownIdCache.end()) {
                    std::tie(max, maxNode, maxState) = iter->second;
                }
            }

            if (!maxNode) {
                State state = d->model_->nullState();
                for (auto &parent : lattice[from]) {
                    auto score =
                        parent.score_ +
                        d->model_->score(parent.state_, node.idx_, state);
                    if (score > max) {
                        max = score;
                        maxNode = &parent;
                        maxState = state;
                    }
                }

                if (node.idx_ == d->model_->unknown()) {
                    unknownIdCache.emplace(
                        std::piecewise_construct, std::forward_as_tuple(from),
                        std::forward_as_tuple(max, maxNode, maxState));
                }
            }

            assert(maxNode);
            node.score_ = max + node.cost_;
            node.prev_ = maxNode;
            node.state_ = std::move(maxState);
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

    // backward search
    if (nbest == 1) {
        assert(lattice[&graph.start()].size() == 1);
        assert(lattice[nullptr].size() == 1);
        auto pos = &lattice[nullptr][0];
        auto bos = &lattice[&graph.start()][0];
        std::string sentence;
        while (pos != bos) {
            sentence = pos->word_ + " " + sentence;
            pos = pos->prev_;
        }
        std::cout << sentence << std::endl;
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

                if (eos->score_ - node->fn_ > max) {
                    break;
                }
                result.push(node);
                if (result.size() >= nbest) {
                    break;
                }
                dup.insert(sentence);
            } else {
                State state = d->model_->nullState();
                for (auto &from : lattice[node->node_->from_]) {
                    auto score = d->model_->score(from.state_,
                                                  node->node_->idx_, state) +
                                 node->node_->cost_;
                    if (&from != bos && score < min)
                        continue;
                    auto parent = newNBestNode(&from);
                    parent->gn_ = score + node->gn_;
                    parent->fn_ = parent->gn_ + parent->node_->score_;
                    parent->next_ = node;

                    q.push(parent);
                }
            }
        }

        while (!result.empty()) {
            auto node = result.top();
            result.pop();
            // TODO
            std::cout << concatNBest(node, " ") << node->fn_ << " "
                      << node->fn_ - eos->score_ << std::endl;
        }
    }
}

void Decoder::setUnknownHandler(UnknownHandler handler) {
    FCITX_D();
    d->unknownHandler_ = handler;
}
}
