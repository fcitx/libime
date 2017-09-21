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

#include "pinyindictionary.h"
#include "libime/core/datrie.h"
#include "libime/core/lattice.h"
#include "libime/core/lrucache.h"
#include "libime/core/utils.h"
#include "pinyindata.h"
#include "pinyindecoder_p.h"
#include "pinyinencoder.h"
#include "pinyinmatchstate_p.h"
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

struct PinyinSegmentGraphPathHasher {
    PinyinSegmentGraphPathHasher(const SegmentGraph &graph) : graph_(graph) {}

    // Generate a "|" separated raw pinyin string from given path, skip all
    // separator.
    std::string pathToPinyins(const SegmentGraphPath &path) const {
        std::string result;
        result.reserve(path.size() + path.back()->index() -
                       path.front()->index() + 1);
        const auto &data = graph_.data();
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

    // Generate hash for path but avoid allocate the string.
    size_t operator()(const SegmentGraphPath &path) const {
        if (path.size() <= 1) {
            return 0;
        }
        boost::hash<char> hasher;

        size_t seed = 0;
        const auto &data = graph_.data();
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

    // Check equality of pinyin string and the path. The string s should be
    // equal to pathToPinyins(path), but this function just try to avoid
    // allocate a string for comparisin.
    bool operator()(const SegmentGraphPath &path, const std::string &s) const {
        auto is = s.begin();
        const auto &data = graph_.data();
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

private:
    const SegmentGraph &graph_;
};

struct SegmentGraphNodeGreater {
    bool operator()(const SegmentGraphNode *lhs,
                    const SegmentGraphNode *rhs) const {
        return lhs->index() > rhs->index();
    }
};

// Check if the prev not is a pinyin. Separator always contrains in its own
// segment.
const SegmentGraphNode *prevIsSeparator(const SegmentGraph &graph,
                                        const SegmentGraphNode &node) {
    if (node.prevSize() == 1) {
        auto &prev = node.prevs().front();
        auto pinyin = graph.segment(prev, node);
        if (boost::starts_with(pinyin, "\'")) {
            return &prev;
        }
    }
    return nullptr;
}

class PinyinMatchContext {
public:
    explicit PinyinMatchContext(
        const SegmentGraph &graph, const GraphMatchCallback &callback,
        const std::unordered_set<const SegmentGraphNode *> &ignore,
        PinyinMatchState *matchState)
        : graph_(graph), hasher_(graph), callback_(callback), ignore_(ignore),
          matchedPathsMap_(&matchState->d_func()->matchedPaths_),
          nodeCacheMap_(&matchState->d_func()->nodeCacheMap_),
          matchCacheMap_(&matchState->d_func()->matchCacheMap_),
          flags_(matchState->fuzzyFlags()),
          spProfile_(matchState->shuangpinProfile()) {}

    explicit PinyinMatchContext(
        const SegmentGraph &graph, const GraphMatchCallback &callback,
        const std::unordered_set<const SegmentGraphNode *> &ignore,
        NodeToMatchedPinyinPathsMap &matchedPaths)
        : graph_(graph), hasher_(graph), callback_(callback), ignore_(ignore),
          matchedPathsMap_(&matchedPaths) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(PinyinMatchContext);

    const SegmentGraph &graph_;
    PinyinSegmentGraphPathHasher hasher_;

    const GraphMatchCallback &callback_;
    const std::unordered_set<const SegmentGraphNode *> &ignore_;
    NodeToMatchedPinyinPathsMap *matchedPathsMap_;
    PinyinTrieNodeCache *nodeCacheMap_ = nullptr;
    PinyinMatchResultCache *matchCacheMap_ = nullptr;
    PinyinFuzzyFlags flags_{PinyinFuzzyFlag::None};
    std::shared_ptr<const ShuangpinProfile> spProfile_;
};

class PinyinDictionaryPrivate : fcitx::QPtrHolder<PinyinDictionary> {
public:
    PinyinDictionaryPrivate(PinyinDictionary *q)
        : fcitx::QPtrHolder<PinyinDictionary>(q) {}

    FCITX_DEFINE_SIGNAL_PRIVATE(PinyinDictionary, dictionaryChanged);

    void addEmptyMatch(const PinyinMatchContext &context,
                       const SegmentGraphNode &currentNode,
                       MatchedPinyinPaths &currentMatches) const;

    void findMatchesBetween(const PinyinMatchContext &context,
                            const SegmentGraphNode &prevNode,
                            const SegmentGraphNode &currentNode,
                            MatchedPinyinPaths &currentMatches) const;

    bool matchWords(const PinyinMatchContext &context,
                    const MatchedPinyinPaths &newPaths) const;
    bool matchWordsForOnePath(const PinyinMatchContext &context,
                              const MatchedPinyinPath &path) const;

    void matchNode(const PinyinMatchContext &context,
                   const SegmentGraphNode &currentNode) const;

    boost::ptr_vector<PinyinTrie> tries_;
};

void PinyinDictionaryPrivate::addEmptyMatch(
    const PinyinMatchContext &context, const SegmentGraphNode &currentNode,
    MatchedPinyinPaths &currentMatches) const {
    const SegmentGraph &graph = context.graph_;
    // Create a new starting point for current node, and put it in matchResult.
    if (&currentNode != &graph.end() &&
        !boost::starts_with(
            graph.segment(currentNode.index(), currentNode.index() + 1),
            "\'")) {
        SegmentGraphPath vec;
        if (auto prev = prevIsSeparator(graph, currentNode)) {
            vec.push_back(prev);
        }

        vec.push_back(&currentNode);
        for (auto &trie : tries_) {
            currentMatches.emplace_back(&trie, 0, vec);
            currentMatches.back().triePositions().emplace_back(0, 0);
        }
    }
}

PinyinTriePositions
traverseAlongPathOneStepBySyllables(const MatchedPinyinPath &path,
                                    const MatchedPinyinSyllables &syls) {
    PinyinTriePositions positions;
    for (const auto &pr : path.triePositions()) {
        uint64_t _pos;
        size_t fuzzies;
        std::tie(_pos, fuzzies) = pr;
        for (auto &syl : syls) {
            // make a copy
            auto pos = _pos;
            auto initial = static_cast<char>(syl.first);
            auto result = path.trie()->traverse(&initial, 1, pos);
            if (PinyinTrie::isNoPath(result)) {
                continue;
            }
            const auto &finals = syl.second;

            auto updateNext = [fuzzies, &path, &positions](auto finalPair,
                                                           auto pos) {
                auto final = static_cast<char>(finalPair.first);
                auto result = path.trie()->traverse(&final, 1, pos);

                if (!PinyinTrie::isNoPath(result)) {
                    size_t newFuzzies = fuzzies + (finalPair.second ? 1 : 0);
                    positions.emplace_back(pos, newFuzzies);
                }
            };
            if (finals.size() > 1 || finals[0].first != PinyinFinal::Invalid) {
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
    return positions;
}

template <typename T>
void matchWordsOnTrie(const MatchedPinyinPath &path, const T &callback) {
    const char sep = pinyinHanziSep;
    for (auto &pr : path.triePositions()) {
        uint64_t pos;
        size_t fuzzies;
        std::tie(pos, fuzzies) = pr;
        float extraCost = fuzzies * fuzzyCost;
        auto result = path.trie()->traverse(&sep, 1, pos);
        if (PinyinTrie::isNoPath(result)) {
            continue;
        }

        path.trie()->foreach(
            [&path, &callback, extraCost](PinyinTrie::value_type value,
                                          size_t len, uint64_t pos) {
                std::string s;
                s.reserve(len + path.size() * 2 + 1);
                path.trie()->suffix(s, len + path.size() * 2 + 1, pos);
                boost::string_view view(s);
                auto encodedPinyin = view.substr(0, path.size() * 2);
                auto hanzi = view.substr(path.size() * 2 + 1);
                callback(encodedPinyin, hanzi, value + extraCost);
                return true;
            },
            pos);
    }
}

bool PinyinDictionaryPrivate::matchWordsForOnePath(
    const PinyinMatchContext &context, const MatchedPinyinPath &path) const {
    bool matched = false;
    assert(path.path_.size() >= 2);
    const SegmentGraphNode &prevNode = *path.path_[path.path_.size() - 2];
    if (context.matchCacheMap_) {
        auto &matchCache = (*context.matchCacheMap_)[path.trie()];
        auto result =
            matchCache.find(path.path_, context.hasher_, context.hasher_);
        if (!result) {
            result =
                matchCache.insert(context.hasher_.pathToPinyins(path.path_));
            result->clear();

            auto &items = *result;
            matchWordsOnTrie(path,
                             [&items](boost::string_view encodedPinyin,
                                      boost::string_view hanzi, float cost) {
                                 items.emplace_back(hanzi, cost, encodedPinyin);
                             });
        }
        for (auto &item : *result) {
            context.callback_(path.path_, item.word_, item.value_,
                              std::make_unique<PinyinLatticeNodePrivate>(
                                  item.encodedPinyin_));
            if (path.size() == 1 &&
                path.path_[path.path_.size() - 2] == &prevNode) {
                matched = true;
            }
        }
    } else {
        matchWordsOnTrie(path, [&matched, &path, &context, &prevNode](
                                   boost::string_view encodedPinyin,
                                   boost::string_view hanzi, float cost) {
            WordNode word(hanzi, InvalidWordIndex);
            context.callback_(
                path.path_, word, cost,
                std::make_unique<PinyinLatticeNodePrivate>(encodedPinyin));
            if (path.size() == 1 &&
                path.path_[path.path_.size() - 2] == &prevNode) {
                matched = true;
            }
        });
    }

    return matched;
}

bool PinyinDictionaryPrivate::matchWords(
    const PinyinMatchContext &context,
    const MatchedPinyinPaths &newPaths) const {
    bool matched = false;
    for (const auto &path : newPaths) {
        matched |= matchWordsForOnePath(context, path);
    }

    return matched;
}

void PinyinDictionaryPrivate::findMatchesBetween(
    const PinyinMatchContext &context, const SegmentGraphNode &prevNode,
    const SegmentGraphNode &currentNode,
    MatchedPinyinPaths &currentMatches) const {
    const SegmentGraph &graph = context.graph_;
    auto &matchedPathsMap = *context.matchedPathsMap_;
    auto pinyin = graph.segment(prevNode, currentNode);
    // If predecessor is a separator, just copy every existing match result
    // over and don't traverse on the trie.
    if (boost::starts_with(pinyin, "\'")) {
        const auto &prevMatches = matchedPathsMap[&prevNode];
        for (auto &match : prevMatches) {
            // copy the path, and append current node.
            auto path = match.path_;
            path.push_back(&currentNode);
            currentMatches.emplace_back(match.result_, std::move(path));
        }
        // If the last segment is separator, there
        if (&currentNode == &graph.end()) {
            WordNode word("", 0);
            context.callback_({&prevNode, &currentNode}, word, 0, nullptr);
        }
        return;
    }

    const auto syls =
        context.spProfile_
            ? PinyinEncoder::shuangpinToSyllables(pinyin, *context.spProfile_,
                                                  context.flags_)
            : PinyinEncoder::stringToSyllables(pinyin, context.flags_);
    const MatchedPinyinPaths &prevMatchedPaths = matchedPathsMap[&prevNode];
    MatchedPinyinPaths newPaths;
    for (auto &path : prevMatchedPaths) {
        // Make a copy of path so we can modify based on it.
        auto segmentPath = path.path_;
        segmentPath.push_back(&currentNode);

        if (context.nodeCacheMap_) {
            auto &nodeCache = (*context.nodeCacheMap_)[path.trie()];
            auto p =
                nodeCache.find(segmentPath, context.hasher_, context.hasher_);
            std::shared_ptr<MatchedPinyinTrieNodes> result;
            if (!p) {
                result = std::make_shared<MatchedPinyinTrieNodes>(
                    path.trie(), path.size() + 1);
                nodeCache.insert(context.hasher_.pathToPinyins(segmentPath),
                                 result);
                result->triePositions_ =
                    traverseAlongPathOneStepBySyllables(path, syls);
            } else {
                result = *p;
                assert(result->size_ == path.size() + 1);
            }

            if (result->triePositions_.size()) {
                newPaths.emplace_back(result, segmentPath);
            }
        } else {
            // make an empty one
            newPaths.emplace_back(path.trie(), path.size() + 1, segmentPath);

            newPaths.back().result_->triePositions_ =
                traverseAlongPathOneStepBySyllables(path, syls);
            // if there's nothing, pop it.
            if (!newPaths.back().triePositions().size()) {
                newPaths.pop_back();
            }
        }
    }

    if (!context.ignore_.count(&currentNode)) {
        // after we match current syllable, we first try to match word.
        if (!matchWords(context, newPaths)) {
            // If we failed to match any length 1 word, add a new empty word
            // to make lattice connect together.
            SegmentGraphPath vec;
            vec.reserve(3);
            if (auto prevPrev = prevIsSeparator(context.graph_, prevNode)) {
                vec.push_back(prevPrev);
            }
            vec.push_back(&prevNode);
            vec.push_back(&currentNode);
            WordNode word(pinyin, InvalidWordIndex);
            context.callback_(vec, word, invalidPinyinCost, nullptr);
        }
    }

    std::move(newPaths.begin(), newPaths.end(),
              std::back_inserter(currentMatches));
}

void PinyinDictionaryPrivate::matchNode(
    const PinyinMatchContext &context,
    const SegmentGraphNode &currentNode) const {
    auto &matchedPathsMap = *context.matchedPathsMap_;
    // Check if the node has been searched already.
    if (matchedPathsMap.count(&currentNode)) {
        return;
    }
    auto &currentMatches = matchedPathsMap[&currentNode];
    // To create a new start.
    addEmptyMatch(context, currentNode, currentMatches);

    // Iterate all predecessor and search from them.
    for (auto &prevNode : currentNode.prevs()) {
        findMatchesBetween(context, prevNode, currentNode, currentMatches);
    }
}

void PinyinDictionary::matchPrefixImpl(
    const SegmentGraph &graph, const GraphMatchCallback &callback,
    const std::unordered_set<const SegmentGraphNode *> &ignore,
    void *helper) const {
    FCITX_D();

    NodeToMatchedPinyinPathsMap localMatchedPaths;
    PinyinMatchContext context =
        helper ? PinyinMatchContext{graph, callback, ignore,
                                    static_cast<PinyinMatchState *>(helper)}
               : PinyinMatchContext{graph, callback, ignore, localMatchedPaths};

    // A queue to make sure that node with smaller index will be visted first
    // because we want to make sure every predecessor node are visited before
    // visit the current node.
    using SegmentGraphNodeQueue =
        std::priority_queue<const SegmentGraphNode *,
                            std::vector<const SegmentGraphNode *>,
                            SegmentGraphNodeGreater>;
    SegmentGraphNodeQueue q;

    auto &start = graph.start();
    q.push(&start);

    while (!q.empty()) {
        auto currentNode = q.top();
        q.pop();

        // Push successors into the queue.
        for (auto &node : currentNode->nexts()) {
            q.push(&node);
        }

        d->matchNode(context, *currentNode);
    }
}

void PinyinDictionary::matchWords(const char *data, size_t size,
                                  PinyinMatchCallback callback) const {
    FCITX_D();
    for (size_t i = 0; i < size / 2; i++) {
        if (!PinyinEncoder::isValidInitial(data[i * 2])) {
            throw std::invalid_argument("invalid pinyin");
        }
    }

    std::list<std::pair<const PinyinTrie *, PinyinTrie::position_type>> nodes;
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
        node.first->foreach(
            [&node, &callback, size](PinyinTrie::value_type value, size_t len,
                                     uint64_t pos) {
                std::string s;
                node.first->suffix(s, len + size + 1, pos);

                auto view = boost::string_view(s);
                return callback(s.substr(0, size), view.substr(size + 1),
                                value);
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

void PinyinDictionary::save(size_t idx, const char *filename,
                            PinyinDictFormat format) {
    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    throw_if_io_fail(fout);
    save(idx, fout, format);
}

void PinyinDictionary::save(size_t idx, std::ostream &out,
                            PinyinDictFormat format) {
    FCITX_D();
    switch (format) {
    case PinyinDictFormat::Text:
        saveText(idx, out);
        break;
    case PinyinDictFormat::Binary:
        d->tries_[idx].save(out);
        break;
    default:
        throw std::invalid_argument("invalid format type");
    }
}

void PinyinDictionary::saveText(size_t idx, std::ostream &out) {
    FCITX_D();
    std::string buf;
    std::ios state(nullptr);
    state.copyfmt(out);
    auto &trie = d->tries_[idx];
    trie.foreach([this, &trie, &buf, &out](float value, size_t _len,
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
}
