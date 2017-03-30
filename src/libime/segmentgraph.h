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

#include <boost/multi_index/global_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/utility/string_view.hpp>
#include <functional>
#include <list>
#include <unordered_map>

namespace libime {

class SegmentGraph;
typedef std::function<bool(const SegmentGraph &, const std::vector<size_t> &)>
    SegmentGraphDFSCallback;

class SegmentGraphNode;

class SegmentGraphNode {
public:
    SegmentGraphNode(size_t start) : start_(start) {}
    const auto &next() const { return next_; }
    void addEdge(SegmentGraphNode &ref) {
        assert(ref.start_ > start_);
        next_.push_back(std::ref(ref));
    }
    void removeEdge(SegmentGraphNode &ref) {
        auto &index = boost::multi_index::get<1>(next_);
        index.erase(&ref);
    }

    size_t index() const { return start_; }

    static const SegmentGraphNode *
    referenceAddress(const std::reference_wrapper<SegmentGraphNode> &wrapper) {
        return &wrapper.get();
    }

    bool operator==(const SegmentGraphNode &other) const {
        return this == &other;
    }
    bool operator!=(const SegmentGraphNode &other) const {
        return !operator==(other);
    }

private:
    boost::multi_index_container<
        std::reference_wrapper<SegmentGraphNode>,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            boost::multi_index::hashed_unique<boost::multi_index::global_fun<
                const std::reference_wrapper<SegmentGraphNode> &,
                const SegmentGraphNode *,
                &SegmentGraphNode::referenceAddress>>>>
        next_;
    size_t start_;
};

class SegmentGraph {
public:
    SegmentGraph(const std::string &data = {}) : data_(data) {
        newNode(0);
        newNode(data_.size());
    }
    SegmentGraph(const SegmentGraph &seg) = default;
    ~SegmentGraph() {}

    SegmentGraphNode &start() { return graph_.find(0)->second.front(); }
    SegmentGraphNode &end() {
        return graph_.find(data_.size())->second.front();
    }

    const SegmentGraphNode &start() const {
        return graph_.find(0)->second.front();
    }
    const SegmentGraphNode &end() const {
        return graph_.find(data_.size())->second.front();
    }

    SegmentGraphNode &newNode(size_t idx) {
        graph_[idx].emplace_back(idx);
        return graph_[idx].back();
    }

    const auto &nodes(size_t idx) { return graph_[idx]; }

    auto &node(size_t idx) {
        auto &v = graph_[idx];
        if (v.empty()) {
            v.emplace_back(idx);
        }
        return v.back();
    }

    // helper for dag style segments
    void addNext(size_t from, size_t to) {
        assert(from < to);
        assert(to <= data_.size());
        node(from).addEdge(node(to));
    }

    const std::string &data() const { return data_; }

    boost::string_view segment(size_t start, size_t end) const {
        assert(start < end);
        return boost::string_view(data_.data() + start, end - start);
    }

    void dfs(SegmentGraphDFSCallback callback) const {
        std::vector<size_t> path;
        dfsHelper(path, start(), callback);
    }

private:
    bool dfsHelper(std::vector<size_t> &path, const SegmentGraphNode &start,
                   SegmentGraphDFSCallback callback) const {
        if (start == end()) {
            return callback(*this, path);
        }
        auto &nexts = start.next();
        for (auto next : nexts) {
            auto idx = next.get().index();
            path.push_back(idx);
            if (!dfsHelper(path, next, callback)) {
                return false;
            }
            path.pop_back();
        }
        return true;
    }

    std::string data_;
    std::unordered_map<size_t, std::list<SegmentGraphNode>> graph_;
};
}

#endif // _FCITX_LIBIME_SEGMENTGRAPH_H_
