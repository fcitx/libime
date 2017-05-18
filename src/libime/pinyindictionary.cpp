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

#include "pinyindictionary.h"
#include "datrie.h"
#include "lattice.h"
#include "lrucache.h"
#include "pinyindata.h"
#include "pinyinencoder.h"
#include "utils.h"
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/unordered_map.hpp>
#include <boost/utility/string_view.hpp>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <queue>
#include <type_traits>

namespace libime {

static const float fuzzyCost = std::log10(0.5f);
static const float invalidPinyinCost = -100.0f;
static const char pinyinHanziSep = '!';

struct MatchResult {
    MatchResult(const PinyinTrie *trie, size_t size)
        : trie_(trie), size_(size) {}

    MatchResult(const MatchResult &) = default;
    MatchResult(MatchResult &&) = default;

    MatchResult &operator=(MatchResult &&) = default;
    MatchResult &operator=(const MatchResult &) = default;

    const PinyinTrie *trie_;
    std::vector<std::pair<uint64_t, size_t>> pos_;
    size_t size_;
};

struct MatchItem {
    MatchItem(boost::string_view s, float value,
              boost::string_view encodedPinyin)
        : word_(s, InvalidWordIndex), value_(value),
          encodedPinyin_(encodedPinyin.to_string()) {}
    WordNode word_;
    float value_;
    std::string encodedPinyin_;
};

struct TrieEdge {
    TrieEdge(const PinyinTrie *trie, size_t size, SegmentGraphPath path)
        : result_(std::make_shared<MatchResult>(trie, size)),
          path_(std::move(path)) {}

    TrieEdge(std::shared_ptr<MatchResult> result, SegmentGraphPath path)
        : result_(result), path_(std::move(path)) {}

    TrieEdge(const TrieEdge &) = default;
    TrieEdge(TrieEdge &&) = default;

    TrieEdge &operator=(TrieEdge &&) = default;
    TrieEdge &operator=(const TrieEdge &) = default;

    auto &pos() { return result_->pos_; }
    const auto &pos() const { return result_->pos_; }
    const PinyinTrie *trie() const { return result_->trie_; }
    auto size() const { return result_->size_; }

    std::shared_ptr<MatchResult> result_;
    SegmentGraphPath path_;
};

typedef std::unordered_map<const SegmentGraphNode *, std::vector<TrieEdge>>
    MatchStateMap;

struct SegmentGraphPathHasher {
    SegmentGraphPathHasher(const SegmentGraph *graph) : graph_(graph) {}

    std::string pathToPinyins(const SegmentGraphPath &path) {
        std::string result;
        result.reserve(path.size() + path.back()->index() -
                       path.front()->index() + 1);
        auto data = graph_->data();
        auto iter = path.begin();
        while (iter + 1 < path.end()) {
            auto begin = (*iter)->index();
            auto end = (*std::next(iter))->index();
            iter++;
            if (data[begin] == '\'') {
                continue;
            }
            while (begin < end) {
                result.push_back(data[begin]);
                begin++;
            }
            result.push_back('|');
        }
        return result;
    }

    size_t operator()(const SegmentGraphPath &path) const {
        if (path.size() <= 1) {
            return 0;
        }
        boost::hash<char> hasher;

        size_t seed = 0;
        auto data = graph_->data();
        auto iter = path.begin();
        while (iter + 1 < path.end()) {
            auto begin = (*iter)->index();
            auto end = (*std::next(iter))->index();
            iter++;
            if (data[begin] == '\'') {
                continue;
            }
            while (begin < end) {
                boost::hash_combine(seed, hasher(data[begin]));
                begin++;
            }
            boost::hash_combine(seed, hasher('|'));
        }
        return seed;
    }

    bool operator()(const SegmentGraphPath &path, const std::string &s) const {
        auto is = s.begin();
        auto data = graph_->data();
        auto iter = path.begin();
        while (iter + 1 < path.end() && is != s.end()) {
            auto begin = (*iter)->index();
            auto end = (*std::next(iter))->index();
            iter++;
            if (data[begin] == '\'') {
                continue;
            }
            while (begin < end && is != s.end()) {
                if (*is != data[begin]) {
                    return false;
                }
                is++;
                begin++;
            }
            if (begin != end) {
                return false;
            }

            if (is == s.end() || *is != '|') {
                return false;
            }
            is++;
        }
        return is == s.end();
    }

    const SegmentGraph *graph_;
};

typedef std::unordered_map<const PinyinTrie *,
                           LRUCache<std::string, std::shared_ptr<MatchResult>>>
    MatchNodeCache;
typedef std::unordered_map<const PinyinTrie *,
                           LRUCache<std::string, std::vector<MatchItem>>>
    MatchCache;

class PinyinMatchStatePrivate {
public:
    PinyinMatchStatePrivate(PinyinDictionary *dict) : dict_(dict) {}

    PinyinDictionary *dict_;
    MatchStateMap search_;
    MatchNodeCache nodeCache_;
    MatchCache matchCache_;
};

PinyinMatchState::PinyinMatchState(PinyinDictionary *dict)
    : d_ptr(std::make_unique<PinyinMatchStatePrivate>(dict)) {}
PinyinMatchState::~PinyinMatchState() {}

void PinyinMatchState::clear() {
    FCITX_D();
    d->search_.clear();
    d->nodeCache_.clear();
    d->matchCache_.clear();
}

void PinyinMatchState::discardNode(
    const std::unordered_set<const SegmentGraphNode *> &nodes) {
    FCITX_D();
    for (auto node : nodes) {
        d->search_.erase(node);
    }
    for (auto &p : d->search_) {
        auto &l = p.second;
        auto iter = l.begin();
        while (iter != l.end()) {
            if (nodes.count(iter->path_.front())) {
                iter = l.erase(iter);
            } else {
                iter++;
            }
        }
    }
}

void PinyinMatchState::discardDictionary(size_t idx) {
    FCITX_D();
    d->matchCache_.erase(d->dict_->trie(idx));
    d->nodeCache_.erase(d->dict_->trie(idx));
}

class PinyinDictionaryPrivate : fcitx::QPtrHolder<PinyinDictionary> {
public:
    PinyinDictionaryPrivate(PinyinDictionary *q)
        : fcitx::QPtrHolder<PinyinDictionary>(q) {}

    FCITX_DEFINE_SIGNAL_PRIVATE(PinyinDictionary, dictionaryChanged);

    boost::ptr_vector<PinyinTrie> tries_;
    PinyinFuzzyFlags flags_;
};

struct SegmentGraphNodeGreater {
    bool operator()(const SegmentGraphNode *lhs,
                    const SegmentGraphNode *rhs) const {
        return lhs->index() > rhs->index();
    }
};

const SegmentGraphNode *prevIsSeparator(const SegmentGraph &graph,
                                        const SegmentGraphNode *node) {
    auto prev = node->prev();
    if (!prev.empty()) {
        auto iter = prev.begin();
        ++iter;
        if (iter == prev.end()) {
            auto i = prev.begin();
            auto pinyin = graph.segment(i->index(), node->index());
            if (boost::starts_with(pinyin, "\'")) {
                return &(*i);
            }
        }
    }
    return nullptr;
}

void PinyinDictionary::matchPrefixImpl(
    const SegmentGraph &graph, GraphMatchCallback callback,
    const std::unordered_set<const SegmentGraphNode *> &ignore, void *helper) {
    FCITX_D();

    MatchStateMap _search;
    MatchStateMap *searchP;
    MatchCache *cache = nullptr;
    MatchNodeCache *nodeCache = nullptr;
    if (helper) {
        searchP = &static_cast<PinyinMatchState *>(helper)->d_func()->search_;
        nodeCache =
            &static_cast<PinyinMatchState *>(helper)->d_func()->nodeCache_;
        cache = &static_cast<PinyinMatchState *>(helper)->d_func()->matchCache_;
    } else {
        searchP = &_search;
    }

    MatchStateMap &search = *searchP;

    std::priority_queue<const SegmentGraphNode *,
                        std::vector<const SegmentGraphNode *>,
                        SegmentGraphNodeGreater>
        q;

    auto &start = graph.start();
    q.push(&start);

    SegmentGraphPathHasher hasher(&graph);

    while (!q.empty()) {
        auto current = q.top();
        q.pop();

        for (auto &node : current->next()) {
            q.push(&node);
        }

        if (search.count(current)) {
            continue;
        }

        auto &currentNodes = search[current];
        if (current != &graph.end() &&
            !boost::starts_with(
                graph.segment(current->index(), current->index() + 1), "\'")) {
            SegmentGraphPath vec;
            if (auto prev = prevIsSeparator(graph, current)) {
                vec.push_back(prev);
            }

            vec.push_back(current);
            for (auto &trie : d->tries_) {
                currentNodes.emplace_back(&trie, 0, vec);
                currentNodes.back().pos().emplace_back(0, 0);
            }
        }

        for (auto &prevNode : current->prev()) {
            auto pinyin = graph.segment(prevNode.index(), current->index());
            if (boost::starts_with(pinyin, "\'")) {
                const auto &nodes = search[&prevNode];
                for (auto &node : nodes) {
                    auto path = node.path_;
                    path.push_back(current);
                    currentNodes.emplace_back(node.result_, std::move(path));
                }
                if (current == &graph.end()) {
                    WordNode word("", 0);
                    callback({&prevNode, current}, word, 0, "");
                }
                continue;
            }

            bool matched = false;
            const auto syls =
                PinyinEncoder::stringToSyllables(pinyin, d->flags_);
            const auto &nodes = search[&prevNode];
            std::remove_cv_t<std::remove_reference_t<decltype(nodes)>> newNodes;
            for (auto &node : nodes) {
                auto v = node.path_;
                v.push_back(current);

                auto updateNode = [&node, &syls, &v](MatchResult &newNode) {
                    assert(newNode.pos_.empty());
                    for (const auto &pr : node.pos()) {
                        uint64_t _pos;
                        size_t fuzzies;
                        std::tie(_pos, fuzzies) = pr;
                        for (auto &syl : syls) {
                            // make a copy
                            auto pos = _pos;
                            auto initial = static_cast<char>(syl.first);
                            auto result =
                                node.trie()->traverse(&initial, 1, pos);
                            if (PinyinTrie::isNoPath(result)) {
                                continue;
                            }
                            const auto &finals = syl.second;

                            auto updateNext = [fuzzies, &node, &newNode,
                                               &v](auto finalPair, auto pos) {
                                auto final = static_cast<char>(finalPair.first);
                                auto result =
                                    node.trie()->traverse(& final, 1, pos);

                                if (!PinyinTrie::isNoPath(result)) {
                                    size_t newFuzzies =
                                        fuzzies + (finalPair.second ? 1 : 0);
                                    newNode.pos_.emplace_back(pos, newFuzzies);
                                }
                            };
                            if (finals.size() > 1 ||
                                finals[0].first != PinyinFinal::Invalid) {
                                for (auto final : finals) {
                                    updateNext(final, pos);
                                }
                            } else {
                                for (char test = PinyinEncoder::firstFinal;
                                     test <= PinyinEncoder::lastFinal; test++) {
                                    updateNext(std::make_pair(test, true), pos);
                                }
                            }
                        }
                    }
                };

                if (nodeCache) {
                    auto &c = (*nodeCache)[node.trie()];
                    auto p = c.find(v, hasher, hasher);
                    std::shared_ptr<MatchResult> result;
                    if (!p) {
                        result = std::make_shared<MatchResult>(node.trie(),
                                                               node.size() + 1);
                        c.insert(hasher.pathToPinyins(v), result);
                        updateNode(*result);
                    } else {
                        result = *p;
                    }
                    assert(result->size_ == node.size() + 1);

                    if (result->pos_.size()) {
                        newNodes.emplace_back(result, v);
                    }
                } else {
                    // make an empty one
                    newNodes.emplace_back(node.trie(), node.size() + 1, v);

                    updateNode(*newNodes.back().result_);
                    // if there's nothing, pop it.
                    if (!newNodes.back().pos().size()) {
                        newNodes.pop_back();
                    }
                }
            }

            if (!ignore.count(current)) {
                // after we match initial, we first try to match final for
                // current word.

                if (cache) {
                    for (const auto &node : newNodes) {
                        auto &c = (*cache)[node.trie()];
                        auto result = c.find(node.path_, hasher, hasher);
                        if (!result) {
                            result = c.insert(hasher.pathToPinyins(node.path_));
                            result->clear();

                            auto &items = *result;
                            for (auto &pr : node.pos()) {
                                char sep = pinyinHanziSep;
                                uint64_t pos;
                                size_t fuzzies;
                                std::tie(pos, fuzzies) = pr;
                                float extraCost = fuzzies * fuzzyCost;
                                auto result =
                                    node.trie()->traverse(&sep, 1, pos);
                                if (PinyinTrie::isNoPath(result)) {
                                    continue;
                                }

                                node.trie()->foreach (
                                    [d, &node, &items, extraCost,
                                     &prevNode](PinyinTrie::value_type value,
                                                size_t len, uint64_t pos) {
                                        std::string s;
                                        s.reserve(len + node.size() * 2 + 1);
                                        node.trie()->suffix(
                                            s, len + node.size() * 2 + 1, pos);
                                        boost::string_view view(s);
                                        auto encodedPinyin =
                                            view.substr(0, node.size() * 2);
                                        items.emplace_back(
                                            view.substr(node.size() * 2 + 1),
                                            value + extraCost, encodedPinyin);
                                        return true;
                                    },
                                    pos);
                            }
                        }
                        for (auto &item : *result) {
                            callback(node.path_, item.word_, item.value_,
                                     item.encodedPinyin_);
                            if (node.size() == 1 &&
                                node.path_[node.path_.size() - 2] ==
                                    &prevNode) {
                                matched = true;
                            }
                        }
                    }
                } else {
                    for (const auto &node : newNodes) {
                        char sep = pinyinHanziSep;
                        for (auto pr : node.pos()) {
                            uint64_t pos;
                            size_t fuzzies;
                            std::tie(pos, fuzzies) = pr;
                            float extraCost = fuzzies * fuzzyCost;
                            auto result = node.trie()->traverse(&sep, 1, pos);
                            if (PinyinTrie::isNoPath(result)) {
                                continue;
                            }

                            node.trie()->foreach (
                                [d, &callback, &node, &graph, &matched,
                                 extraCost,
                                 &prevNode](PinyinTrie::value_type value,
                                            size_t len, uint64_t pos) {
                                    std::string s;
                                    node.trie()->suffix(
                                        s, len + node.size() * 2 + 1, pos);
                                    auto view = boost::string_view(s);
                                    auto encodedPinyin =
                                        view.substr(0, node.size() * 2);
                                    WordNode word(
                                        view.substr(node.size() * 2 + 1),
                                        InvalidWordIndex);
                                    callback(node.path_, word,
                                             value + extraCost, encodedPinyin);
                                    if (node.size() == 1 &&
                                        node.path_[node.path_.size() - 2] ==
                                            &prevNode) {
                                        matched = true;
                                    }
                                    return true;
                                },
                                pos);
                        }
                    }
                }

                if (!matched) {
                    SegmentGraphPath vec;
                    vec.reserve(3);
                    if (auto prevPrev = prevIsSeparator(graph, &prevNode)) {
                        vec.push_back(prevPrev);
                    }
                    vec.push_back(&prevNode);
                    vec.push_back(current);
                    WordNode word(pinyin, InvalidWordIndex);
                    callback(vec, word, invalidPinyinCost, "");
                }
            }

            std::move(newNodes.begin(), newNodes.end(),
                      std::back_inserter(currentNodes));
        }
    }
}

void PinyinDictionary::matchWords(const char *data, size_t size,
                                  PinyinMatchCallback callback) {
    FCITX_D();
    for (size_t i = 0; i < size / 2; i++) {
        if (!PinyinEncoder::isValidInitial(data[i * 2])) {
            throw std::invalid_argument("invalid pinyin");
        }
    }

    std::list<std::pair<PinyinTrie *, PinyinTrie::position_type>> nodes;
    for (auto &trie : d->tries_) {
        nodes.emplace_back(&trie, 0);
    }
    for (size_t i = 0; i <= size && nodes.size(); i++) {
        char current;
        if (i < size) {
            current = data[i];
        } else {
            current = pinyinHanziSep;
        }
        decltype(nodes) extraNodes;
        auto iter = nodes.begin();
        while (iter != nodes.end()) {
            if (current != 0) {
                PinyinTrie::value_type result;
                result = iter->first->traverse(&current, 1, iter->second);

                if (PinyinTrie::isNoPath(result)) {
                    nodes.erase(iter++);
                } else {
                    iter++;
                }
            } else {
                bool changed = false;
                for (char test = PinyinEncoder::firstFinal;
                     test <= PinyinEncoder::lastFinal; test++) {
                    decltype(extraNodes)::value_type p = *iter;
                    auto result = p.first->traverse(&test, 1, p.second);
                    if (!PinyinTrie::isNoPath(result)) {
                        extraNodes.push_back(p);
                        changed = true;
                    }
                }
                if (changed) {
                    *iter = extraNodes.back();
                    extraNodes.pop_back();
                    iter++;
                } else {
                    nodes.erase(iter++);
                }
            }
        }
        nodes.splice(nodes.end(), std::move(extraNodes));
    }

    for (auto &node : nodes) {
        node.first->foreach (
            [&node, &callback, size](PinyinTrie::value_type value, size_t len,
                                     uint64_t pos) {
                std::string s;
                node.first->suffix(s, len + size + 1, pos);
                return callback(s.c_str(), s.substr(size + 1), value);
            },
            node.second);
    }
}
PinyinDictionary::PinyinDictionary()
    : d_ptr(std::make_unique<PinyinDictionaryPrivate>(this)) {
    addEmptyDict();
    addEmptyDict();
}

PinyinDictionary::~PinyinDictionary() {}

void PinyinDictionary::addEmptyDict() {
    FCITX_D();
    d->tries_.push_back(new PinyinTrie);
}

void PinyinDictionary::load(size_t idx, const char *filename,
                            PinyinDictFormat format) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    throw_if_io_fail(in);
    load(idx, in, format);
}

void PinyinDictionary::load(size_t idx, std::istream &in,
                            PinyinDictFormat format) {
    switch (format) {
    case PinyinDictFormat::Text:
        loadText(idx, in);
        break;
    case PinyinDictFormat::Binary:
        loadBinary(idx, in);
        break;
    default:
        throw std::invalid_argument("invalid format type");
    }
    emit<PinyinDictionary::dictionaryChanged>(idx);
}

void PinyinDictionary::loadText(size_t idx, std::istream &in) {
    FCITX_D();
    DATrie<float> trie;

    std::string buf;
    auto isSpaceCheck = boost::is_any_of(" \n\t\r\v\f");
    while (!in.eof()) {
        if (!std::getline(in, buf)) {
            break;
        }

        boost::trim_if(buf, isSpaceCheck);
        std::vector<std::string> tokens;
        boost::split(tokens, buf, isSpaceCheck);
        if (tokens.size() == 3) {
            const std::string &hanzi = tokens[0];
            boost::string_view pinyin = tokens[1];
            float prob = std::stof(tokens[2]);
            auto result = PinyinEncoder::encodeFullPinyin(pinyin.to_string());
            result.push_back(pinyinHanziSep);
            result.insert(result.end(), hanzi.begin(), hanzi.end());
            trie.set(result.data(), result.size(), prob);
        }
    }
    d->tries_[idx] = std::move(trie);
}

void PinyinDictionary::loadBinary(size_t idx, std::istream &in) {
    FCITX_D();
    DATrie<float> trie;
    trie.load(in);
    d->tries_[idx] = std::move(trie);
}

void PinyinDictionary::save(size_t idx, const char *filename) {
    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    throw_if_io_fail(fout);
    save(idx, fout);
}

void PinyinDictionary::save(size_t idx, std::ostream &out) {
    FCITX_D();
    d->tries_[idx].save(out);
}

void PinyinDictionary::dump(size_t idx, std::ostream &out) {
    FCITX_D();
    std::string buf;
    std::ios state(nullptr);
    state.copyfmt(out);
    auto &trie = d->tries_[idx];
    trie.foreach ([this, &trie, &buf, &out](float value, size_t _len,
                                            PinyinTrie::position_type pos) {
        trie.suffix(buf, _len, pos);
        auto sep = buf.find(pinyinHanziSep);
        if (sep == std::string::npos) {
            return true;
        }
        boost::string_view ref(buf);
        auto fullPinyin = PinyinEncoder::decodeFullPinyin(ref.data(), sep);
        out << ref.substr(sep + 1) << " " << fullPinyin << " "
            << std::setprecision(16) << value << std::endl;
        return true;
    });
    out.copyfmt(state);
}

void PinyinDictionary::remove(size_t idx) {
    FCITX_D();
    if (idx <= UserDict) {
        throw std::invalid_argument("User Dict not allow to be removed");
    }
    d->tries_.erase(d->tries_.begin() + idx);
}

const PinyinTrie *PinyinDictionary::trie(size_t idx) const {
    FCITX_D();
    return &d->tries_[idx];
}

size_t PinyinDictionary::dictSize() const {
    FCITX_D();
    return d->tries_.size();
}

void PinyinDictionary::addWord(size_t idx, boost::string_view fullPinyin,
                               boost::string_view hanzi, float cost) {
    FCITX_D();
    auto result = PinyinEncoder::encodeFullPinyin(fullPinyin);
    result.push_back(pinyinHanziSep);
    result.insert(result.end(), hanzi.begin(), hanzi.end());
    d->tries_[idx].set(result.data(), result.size(), cost);
    emit<PinyinDictionary::dictionaryChanged>(idx);
}

void PinyinDictionary::setFuzzyFlags(PinyinFuzzyFlags flags) {
    FCITX_D();
    d->flags_ = flags;
}
}
