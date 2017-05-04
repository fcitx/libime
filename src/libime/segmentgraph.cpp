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

#include "segmentgraph.h"
#include "lattice_p.h"
#include <boost/range/combine.hpp>
#include <boost/algorithm/string.hpp>
#include <queue>
#include <iostream>

namespace libime {

struct SegmentGraphNodePairGreater {
    bool operator()(const std::pair<const SegmentGraphNode *,
                                    const SegmentGraphNode *> &lhs,
                    const std::pair<const SegmentGraphNode *,
                                    const SegmentGraphNode *> &rhs) const {
        return lhs.first->index() > rhs.first->index();
    }
};
struct SegmentGraphNodeGreater {
    bool operator()(const SegmentGraphNode *lhs,
                    const SegmentGraphNode *rhs) const {
        return lhs->index() > rhs->index();
    }
};

void SegmentGraph::bfs(const SegmentGraphNode *from, SegmentGraphBFSCallback callback) const {
    std::priority_queue<const SegmentGraphNode *,
                        std::vector<const SegmentGraphNode *>,
                        SegmentGraphNodeGreater>
        q;
    q.push(from);
    std::unordered_set<const SegmentGraphNode *> visited;
    while (!q.empty()) {
        auto node = q.top();
        q.pop();
        if (!visited.count(node)) {
            visited.insert(node);
        } else {
            continue;
        }

        callback(node);
        for (auto &next : node->next()) {
            q.push(&next);
        }
    }
}

size_t SegmentGraph::check(const SegmentGraph &graph) const {
    for (size_t i = 0; i <= graph.size(); i++) {
        assert(graph.nodes(i).size() <= 1);
    }
    for (size_t i = 0; i <= size(); i++) {
        assert(nodes(i).size() <= 1);
    }

    std::priority_queue<
        std::pair<const SegmentGraphNode *, const SegmentGraphNode *>,
        std::vector<
            std::pair<const SegmentGraphNode *, const SegmentGraphNode *>>,
        SegmentGraphNodePairGreater>
        q;

    q.emplace(&start(), &graph.start());
    while (!q.empty()) {
        const SegmentGraphNode *old, *now;
        std::tie(old, now) = q.top();
        q.pop();
        do {
            assert(old->index() == now->index());
            if (old->nextSize() != now->nextSize()) {
                return old->index();
            }

            const SegmentGraphNode *nold, *nnow;
            for (auto t : boost::combine(old->next(), now->next())) {
                nold = &boost::get<0>(t);
                nnow = &boost::get<1>(t);
                if (nold->index() != nnow->index() ||
                    segment(old->index(), nold->index()) !=
                        graph.segment(now->index(), nnow->index())) {
                    return old->index();
                }
            }

            for (auto t : boost::combine(old->next(), now->next())) {
                nold = &boost::get<0>(t);
                nnow = &boost::get<1>(t);
                q.emplace(nold, nnow);
            }
        } while (0);
    }

    return end().index() + 1;
}

void SegmentGraph::merge(SegmentGraph &graph, DiscardCallback discardCallback) {
    auto since = check(graph);
    std::unordered_set<const SegmentGraphNode *> nodeToDiscard;
    for (size_t i = 0; i < since; i++) {
        auto &ns = nodes(i);
        for (auto &node : ns) {
            std::vector<SegmentGraphNode *> newNext;
            for (auto &next : node.next()) {
                SegmentGraphNode *n;
                if (next.index() >= since) {
                    n = &graph.node(next.index());
                } else {
                    n = &next;
                }
                newNext.push_back(n);
            }
            while (!node.next().empty()) {
                node.removeEdge(node.next().front());
            }
            for (auto n : newNext) {
                node.addEdge(*n);
            }
        }
        graph.graph_[i]->clear();
    }

    data_ = graph.data_;
    resize(data_.size() + 1);
    for (size_t i = since; i <= size(); i++) {
        for (auto &node : nodes(i)) {
            nodeToDiscard.insert(&node);
        }
        std::swap(graph_[i], graph.graph_[i]);
        graph.graph_[i]->clear();
    }

    if (discardCallback) {
        discardCallback(nodeToDiscard);
    }
}
}
