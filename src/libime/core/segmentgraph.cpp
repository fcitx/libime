/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "segmentgraph.h"
#include <boost/range/combine.hpp>
#include <boost/range/detail/combine_cxx11.hpp>
#include <boost/tuple/tuple.hpp>
#include <cassert>
#include <cstddef>
#include <queue>
#include <unordered_set>
#include <utility>
#include <vector>

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

bool SegmentGraphBase::bfs(const SegmentGraphNode *from,
                           const SegmentGraphBFSCallback &callback) const {
    std::priority_queue<const SegmentGraphNode *,
                        std::vector<const SegmentGraphNode *>,
                        SegmentGraphNodeGreater>
        q;
    q.push(from);
    std::unordered_set<const SegmentGraphNode *> visited;
    while (!q.empty()) {
        const auto *node = q.top();
        q.pop();
        if (!visited.contains(node)) {
            visited.insert(node);
        } else {
            continue;
        }

        if (!callback(*this, node)) {
            return false;
        }
        for (const auto &next : node->nexts()) {
            q.push(&next);
        }
    }
    return true;
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
        auto [old, now] = q.top();
        q.pop();
        do {
            assert(old->index() == now->index());
            if (old->nextSize() != now->nextSize()) {
                return old->index();
            }

            const SegmentGraphNode *nold;
            const SegmentGraphNode *nnow;
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
            for (auto *n : newNext) {
                node.addEdge(*n);
            }
        }
        graph.graph_[i].reset();
    }

    mutableData() = graph.data();

    // these nodes will be discarded by resize()
    if (data().size() + 1 < graph_.size()) {
        for (size_t i = data().size() + 1; i < graph_.size(); i++) {
            for (const auto &node : nodes(i)) {
                nodeToDiscard.insert(&node);
            }
        }
    }

    resize(data().size() + 1);
    for (size_t i = since; i <= size(); i++) {
        for (const auto &node : nodes(i)) {
            nodeToDiscard.insert(&node);
        }
        std::swap(graph_[i], graph.graph_[i]);
        graph.graph_[i].reset();
    }

    if (discardCallback) {
        discardCallback(nodeToDiscard);
    }
}
} // namespace libime
