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
#ifndef _FCITX_LIBIME_SEGMENTGRAPH_H_
#define _FCITX_LIBIME_SEGMENTGRAPH_H_

#include "libime_export.h"
#include <boost/iterator/transform_iterator.hpp>
#include <boost/ptr_container/ptr_list.hpp>
#include <boost/range/adaptor/type_erased.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/utility/string_view.hpp>
#include <fcitx-utils/element.h>
#include <functional>
#include <list>
#include <unordered_map>
#include <unordered_set>

namespace libime {

class Lattice;
class SegmentGraph;
class SegmentGraphNode;
typedef std::function<bool(const std::vector<size_t> &)>
    SegmentGraphDFSCallback;
typedef std::function<void(const SegmentGraphNode *)> SegmentGraphBFSCallback;

using SegmentGraphNodeRange =
    boost::any_range<SegmentGraphNode, boost::bidirectional_traversal_tag>;
using SegmentGraphNodeConstRange =
    boost::any_range<const SegmentGraphNode,
                     boost::bidirectional_traversal_tag>;

class LIBIME_EXPORT SegmentGraphNode : public fcitx::Element {
    friend class SegmentGraph;

public:
    SegmentGraphNode(size_t start) : fcitx::Element(), start_(start) {}
    SegmentGraphNode(const SegmentGraphNode &node) = delete;
    virtual ~SegmentGraphNode() {}

    SegmentGraphNodeConstRange nexts() const {
        auto &nexts = childs();
        return boost::make_iterator_range(
            boost::make_transform_iterator(nexts.begin(),
                                           &SegmentGraphNode::castCosnt),
            boost::make_transform_iterator(nexts.end(),
                                           &SegmentGraphNode::castCosnt));
    }

    size_t nextSize() const { return childs().size(); }

    SegmentGraphNodeConstRange prevs() const {
        auto &prevs = parents();
        return boost::make_iterator_range(
            boost::make_transform_iterator(prevs.begin(),
                                           &SegmentGraphNode::castCosnt),
            boost::make_transform_iterator(prevs.end(),
                                           &SegmentGraphNode::castCosnt));
    }

    size_t prevSize() const { return parents().size(); }
    inline size_t index() const { return start_; }

    bool operator==(const SegmentGraphNode &other) const {
        return this == &other;
    }
    bool operator!=(const SegmentGraphNode &other) const {
        return !operator==(other);
    }

protected:
    SegmentGraphNodeRange mutablePrevs() {
        auto &prevs = parents();
        return boost::make_iterator_range(
            boost::make_transform_iterator(prevs.begin(),
                                           &SegmentGraphNode::cast),
            boost::make_transform_iterator(prevs.end(),
                                           &SegmentGraphNode::cast));
    }

    SegmentGraphNodeRange mutableNexts() {
        auto &nexts = childs();
        return boost::make_iterator_range(
            boost::make_transform_iterator(nexts.begin(),
                                           &SegmentGraphNode::cast),
            boost::make_transform_iterator(nexts.end(),
                                           &SegmentGraphNode::cast));
    }

    void addEdge(SegmentGraphNode &ref) {
        assert(ref.start_ > start_);
        addChild(&ref);
    }
    void removeEdge(SegmentGraphNode &ref) { removeChild(&ref); }

    static auto castCosnt(fcitx::Element *ele) -> const SegmentGraphNode & {
        return *static_cast<SegmentGraphNode *>(ele);
    }
    static auto cast(fcitx::Element *ele) -> SegmentGraphNode & {
        return *static_cast<SegmentGraphNode *>(ele);
    }

private:
    size_t start_;
};

typedef std::vector<const SegmentGraphNode *> SegmentGraphPath;
typedef std::function<void(
    const std::unordered_set<const SegmentGraphNode *> &node)>
    DiscardCallback;

class LIBIME_EXPORT SegmentGraphBase {
public:
    SegmentGraphBase(boost::string_view data = {})
        : data_(std::make_unique<std::string>(data.to_string())) {}
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE(SegmentGraphBase)

    virtual const SegmentGraphNode &start() const = 0;
    virtual const SegmentGraphNode &end() const = 0;

    virtual SegmentGraphNodeConstRange nodes(size_t idx) const = 0;
    inline const SegmentGraphNode &node(size_t idx) const {
        return nodes(idx).front();
    }

    // Return the string.
    const std::string &data() const { return *data_; }

    // Return the size of string.
    size_t size() const { return data().size(); }

    inline boost::string_view segment(size_t start, size_t end) const {
        return boost::string_view(data().data() + start, end - start);
    }

    inline boost::string_view segment(const SegmentGraphNode &start,
                                      const SegmentGraphNode &end) const {
        return segment(start.index(), end.index());
    }

    void bfs(const SegmentGraphNode *from,
             const SegmentGraphBFSCallback &callback) const;

    void dfs(const SegmentGraphDFSCallback &callback) const {
        std::vector<size_t> path;
        dfsHelper(path, start(), callback);
    }

    bool isList() const {
        const SegmentGraphNode *node = &start();
        const SegmentGraphNode *endNode = &end();
        while (node != endNode) {
            if (node->nextSize() != 1) {
                return false;
            }
            node = &node->nexts().front();
        }
        return true;
    }

    bool checkGraph() const {
        std::unordered_set<const SegmentGraphNode *> allNodes;
        for (size_t i = 0, e = size(); i < e; i++) {
            for (const auto &n : nodes(i)) {
                if (n.nexts().empty() && n != end()) {
                    return false;
                }
                allNodes.insert(&n);
            }
        }

        bfs(&start(), [&allNodes](const SegmentGraphNode *node) {
            allNodes.erase(node);
        });

        return allNodes.empty();
    }

    bool checkNodeInGraph(const SegmentGraphNode *node) const {
        for (size_t i = 0, e = size(); i <= e; i++) {
            for (const auto &n : nodes(i)) {
                if (&n == node) {
                    return true;
                }
            }
        }
        return false;
    }

protected:
    std::string &mutableData() { return *data_; }

private:
    bool dfsHelper(std::vector<size_t> &path, const SegmentGraphNode &start,
                   const SegmentGraphDFSCallback &callback) const {
        if (start == end()) {
            return callback(path);
        }
        auto nexts = start.nexts();
        for (auto &next : nexts) {
            auto idx = next.index();
            path.push_back(idx);
            if (!dfsHelper(path, next, callback)) {
                return false;
            }
            path.pop_back();
        }
        return true;
    }

    // Temporary workaround for string's move.
    std::unique_ptr<std::string> data_;
};

class LIBIME_EXPORT SegmentGraph : public SegmentGraphBase {
public:
    SegmentGraph(boost::string_view str = {}) : SegmentGraphBase(str) {
        resize(data().size() + 1);
        if (data().size()) {
            newNode(data().size());
        }
        newNode(0);
    }
    SegmentGraph(const SegmentGraph &seg) = delete;
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE(SegmentGraph)

    const SegmentGraphNode &start() const override { return *graph_[0]; }
    const SegmentGraphNode &end() const override {
        return *graph_[data().size()];
    }
    void merge(SegmentGraph &graph,
               const DiscardCallback &discardCallback = {});

    SegmentGraphNode &ensureNode(size_t idx) {
        if (nodes(idx).empty()) {
            newNode(idx);
        }
        return mutableNode(idx);
    }

    SegmentGraphNodeConstRange nodes(size_t idx) const override {
        if (graph_[idx]) {
            return {graph_[idx].get(), graph_[idx].get() + 1};
        } else {
            return {};
        }
    }
    using SegmentGraphBase::node;

    // helper for dag style segments
    void addNext(size_t from, size_t to) {
        assert(from < to);
        assert(to <= data().size());
        if (nodes(from).empty()) {
            newNode(from);
        }
        if (nodes(to).empty()) {
            newNode(to);
        }
        graph_[from]->addEdge(*graph_[to]);
    }

    void appendToLast(boost::string_view str) {
        // append empty string is meaningless.
        if (!str.size()) {
            return;
        }

        size_t oldSize = data().size();
        mutableData().append(str.data(), str.size());
        resize(data().size() + 1);

        newNode(data().size());
        auto &node = mutableNode(oldSize);
        auto &newEnd = mutableNode(data().size());
        // If old size is 0, then just create an edge from begin to end.
        if (oldSize == 0) {
            node.addEdge(newEnd);
            return;
        }

        // Otherwise, iterate the prevs of oldEnd, create a edge between all
        // prevs and newEnd, and delete old end.
        for (auto &prev : node.mutablePrevs()) {
            prev.addEdge(newEnd);
        }
        // Remove the old node.
        graph_[oldSize].reset();
    }

private:
    void resize(size_t newSize) {
        auto oldSize = graph_.size();
        graph_.resize(newSize);
        for (; oldSize < newSize; oldSize++) {
            graph_[oldSize].reset();
        }
    }

    SegmentGraphNode &newNode(size_t idx) {
        graph_[idx] = std::make_unique<SegmentGraphNode>(idx);
        return *graph_[idx];
    }

    inline SegmentGraphNode &mutableNode(size_t idx) {
        return mutableNodes(idx).front();
    }

    SegmentGraphNodeRange mutableNodes(size_t idx) {
        if (graph_[idx]) {
            return {graph_[idx].get(), graph_[idx].get() + 1};
        } else {
            return {};
        }
    }

    size_t check(const SegmentGraph &graph) const;
    // ptr_vector doesn't have move constructor, G-R-E-A-T
    std::vector<std::unique_ptr<SegmentGraphNode>> graph_;
};
}

#endif // _FCITX_LIBIME_SEGMENTGRAPH_H_
