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

#include "segmentgraph.h"
#include "lattice_p.h"
#include <boost/algorithm/string.hpp>
#include <boost/range/combine.hpp>
#include <iostream>
#include <queue>

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

void SegmentGraphBase::bfs(const SegmentGraphNode *from,
                           const SegmentGraphBFSCallback &callback) const {
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
        for (auto &next : node->nexts()) {
            q.push(&next);
        }
    }
}

size_t SegmentGraph::check(const SegmentGraph &graph) const {
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
            for (auto t : boost::combine(old->nexts(), now->nexts())) {
                nold = &boost::get<0>(t);
                nnow = &boost::get<1>(t);
                if (nold->index() != nnow->index() ||
                    segment(*old, *nold) != graph.segment(*now, *nnow)) {
                    return old->index();
                }
            }

            for (auto t : boost::combine(old->nexts(), now->nexts())) {
                nold = &boost::get<0>(t);
                nnow = &boost::get<1>(t);
                q.emplace(nold, nnow);
            }
        } while (0);
    }

    return end().index() + 1;
}

void SegmentGraph::merge(SegmentGraph &graph,
                         const DiscardCallback &discardCallback) {
    if (&graph == this) {
        return;
    }
    auto since = check(graph);
    std::unordered_set<const SegmentGraphNode *> nodeToDiscard;
    for (size_t i = 0; i < since; i++) {
        for (auto &node : mutableNodes(i)) {
            std::vector<SegmentGraphNode *> newNext;
            for (auto &next : node.mutableNexts()) {
                SegmentGraphNode *n;
                if (next.index() >= since) {
                    n = graph.graph_[next.index()].get();
                } else {
                    n = &next;
                }
                newNext.push_back(n);
            }
            while (node.nextSize()) {
                node.removeEdge(node.mutableNexts().front());
            }
            for (auto n : newNext) {
                node.addEdge(*n);
            }
        }
        graph.graph_[i].reset();
    }

    mutableData() = graph.data();

    // these nodes will be discarded by resize()
    if (data().size() + 1 < graph_.size()) {
        for (size_t i = data().size() + 1; i < graph_.size(); i++) {
            for (auto &node : nodes(i)) {
                nodeToDiscard.insert(&node);
            }
        }
    }

    resize(data().size() + 1);
    for (size_t i = since; i <= size(); i++) {
        for (auto &node : nodes(i)) {
            nodeToDiscard.insert(&node);
        }
        std::swap(graph_[i], graph.graph_[i]);
        graph.graph_[i].reset();
    }

    if (discardCallback) {
        discardCallback(nodeToDiscard);
    }
}
}
