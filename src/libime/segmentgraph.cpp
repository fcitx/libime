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

    return graph.end().index() + 1;
}

void SegmentGraph::merge(SegmentGraph &graph, size_t since, Lattice &lattice) {
    for (size_t i = 0; i < since; i++) {
        for (auto &node : nodes(i)) {
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
            lattice.d_ptr->lattice_.erase(&node);
        }
        std::swap(graph_[i], graph.graph_[i]);
        graph.graph_[i]->clear();
    }
}
}
