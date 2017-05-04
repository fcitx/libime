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
typedef std::function<bool(const SegmentGraph &, const std::vector<size_t> &)>
    SegmentGraphDFSCallback;
typedef std::function<void(const SegmentGraphNode *)>
    SegmentGraphBFSCallback;

class LIBIME_EXPORT SegmentGraphNode : public fcitx::Element {
    friend class SegmentGraph;
public:
    SegmentGraphNode(size_t start) : fcitx::Element(), start_(start) {}
    SegmentGraphNode(const SegmentGraphNode &node) = delete;
    virtual ~SegmentGraphNode() {}

    static auto cast(fcitx::Element *ele) -> SegmentGraphNode & {
        return *static_cast<SegmentGraphNode *>(ele);
    }

    typedef boost::any_range<SegmentGraphNode,
                             boost::bidirectional_traversal_tag>
        NodeRange;

    NodeRange next() const {
        auto &nexts = childs();
        return boost::make_iterator_range(
            boost::make_transform_iterator(nexts.begin(),
                                           &SegmentGraphNode::cast),
            boost::make_transform_iterator(nexts.end(),
                                           &SegmentGraphNode::cast));
    }

    size_t nextSize() const { return childs().size(); }

    NodeRange prev() const {
        auto &nexts = parents();
        return boost::make_iterator_range(
            boost::make_transform_iterator(nexts.begin(),
                                           &SegmentGraphNode::cast),
            boost::make_transform_iterator(nexts.end(),
                                           &SegmentGraphNode::cast));
    }

    size_t prevSize() const { return parents().size(); }

    void addEdge(SegmentGraphNode &ref) {
        assert(ref.start_ > start_);
        addChild(&ref);
    }
    void removeEdge(SegmentGraphNode &ref) { removeChild(&ref); }

    inline size_t index() const { return start_; }

    bool operator==(const SegmentGraphNode &other) const {
        return this == &other;
    }
    bool operator!=(const SegmentGraphNode &other) const {
        return !operator==(other);
    }

private:
    size_t start_;
};

typedef std::vector<const SegmentGraphNode *> SegmentGraphPath;
typedef std::function<void(const std::unordered_set<const SegmentGraphNode *> &node)> DiscardCallback;

class LIBIME_EXPORT SegmentGraph {
public:
    SegmentGraph(const std::string &data = {}) : data_(data) {
        resize(data.size() + 1);
        if (data_.size()) {
            newNode(data_.size());
        }
        newNode(0);
    }
    SegmentGraph(const SegmentGraph &seg) = delete;
    SegmentGraph(SegmentGraph &&seg) = default;
    ~SegmentGraph() {}

    SegmentGraph &operator=(SegmentGraph &&seg) = default;

    SegmentGraphNode &start() { return graph_[0]->front(); }
    SegmentGraphNode &end() { return graph_[data_.size()]->front(); }

    const SegmentGraphNode &start() const { return graph_[0]->front(); }
    const SegmentGraphNode &end() const {
        return graph_[data_.size()]->front();
    }
    void merge(SegmentGraph &graph, DiscardCallback discardCallback = {});

    template <typename NodeType = SegmentGraphNode, typename... Args>
    NodeType &newNode(Args &&... args) {
        auto node = new NodeType(std::forward<Args>(args)...);
        graph_[node->index()]->push_back(node);
        return *node;
    }

    auto &nodes(size_t idx) { return *graph_[idx]; }
    const auto &nodes(size_t idx) const { return *graph_[idx]; }

    auto &node(size_t idx) { return graph_[idx]->front(); }

    auto &node(size_t idx) const { return graph_[idx]->front(); }

    // helper for dag style segments
    void addNext(size_t from, size_t to) {
        assert(from < to);
        assert(to <= data_.size());
        node(from).addEdge(node(to));
    }

    const std::string &data() const { return data_; }
    size_t size() const { return data_.size(); }

    inline boost::string_view segment(size_t start, size_t end) const {
        return boost::string_view(data_.data() + start, end - start);
    }

    void bfs(const SegmentGraphNode *from, SegmentGraphBFSCallback callback) const;

    void dfs(SegmentGraphDFSCallback callback) const {
        std::vector<size_t> path;
        dfsHelper(path, start(), callback);
    }

    void setData(boost::string_view data) {
        resize(data.size() + 1);
        data_ = data.to_string();
        if (data_.size()) {
            graph_[data.size()]->clear();
            newNode(data_.size());
        }
    }

private:
    bool dfsHelper(std::vector<size_t> &path, const SegmentGraphNode &start,
                   SegmentGraphDFSCallback callback) const {
        if (start == end()) {
            return callback(*this, path);
        }
        auto nexts = start.next();
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

    void resize(size_t newSize) {
        auto oldSize = graph_.size();
        graph_.resize(newSize);
        for (; oldSize < newSize; oldSize++) {
            graph_[oldSize] =
                std::make_unique<boost::ptr_list<SegmentGraphNode>>();
        }
    }

    size_t check(const SegmentGraph &graph) const;

    std::string data_;
    // ptr_vector doesn't have move constructor, G-R-E-A-T
    std::vector<std::unique_ptr<boost::ptr_list<SegmentGraphNode>>> graph_;
};
}

#endif // _FCITX_LIBIME_SEGMENTGRAPH_H_
