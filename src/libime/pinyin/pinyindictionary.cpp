/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "pinyindictionary.h"
#include "constants.h"
#include "libime/core/datrie.h"
#include "libime/core/dictionary.h"
#include "libime/core/languagemodel.h"
#include "libime/core/lattice.h"
#include "libime/core/lrucache.h"
#include "libime/core/segmentgraph.h"
#include "libime/core/triedictionary.h"
#include "libime/core/utils.h"
#include "libime/core/zstdfilter.h"
#include "libime/pinyin/pinyinmatchstate.h"
#include "pinyindecoder_p.h"
#include "pinyinencoder.h"
#include "pinyinmatchstate_p.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/container_hash/hash.hpp>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/signals.h>
#include <fstream>
#include <iomanip>
#include <ios>
#include <istream>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <ostream>
#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace libime {

namespace {
const float fuzzyCost = std::log10(0.5F);
const size_t minimumLongWordLength = 3;
const float invalidPinyinCost = -100.0F;
const char pinyinHanziSep = '!';

constexpr uint32_t pinyinBinaryFormatMagic = 0x000fc613;
constexpr uint32_t pinyinBinaryFormatVersion = 0x2;

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
    // allocate a string for comparison.
    bool operator()(const SegmentGraphPath &path, const std::string &s) const {
        if (path.size() <= 1) {
            return false;
        }
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
        return iter + 1 == path.end() && is == s.end();
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
        const auto range = node.prevs();
        const auto &prev = range.front();
        auto pinyin = graph.segment(prev, node);
        if (boost::starts_with(pinyin, "\'")) {
            return &prev;
        }
    }
    return nullptr;
}

inline void searchOneStep(
    std::list<std::pair<const PinyinTrie *, PinyinTrie::position_type>> &nodes,
    char current) {
    std::list<std::pair<const PinyinTrie *, PinyinTrie::position_type>>
        extraNodes;
    auto iter = nodes.begin();
    while (iter != nodes.end()) {
        if (current != 0) {
            const auto resultRaw =
                iter->first->traverseRaw(&current, 1, iter->second);

            if (PinyinTrie::isNoPathRaw(resultRaw)) {
                nodes.erase(iter++);
            } else {
                iter++;
            }
        } else {
            bool changed = false;
            for (char test = PinyinEncoder::firstFinal;
                 test <= PinyinEncoder::lastFinal; test++) {
                decltype(extraNodes)::value_type p = *iter;
                const auto resultRaw = p.first->traverseRaw(&test, 1, p.second);
                if (!PinyinTrie::isNoPathRaw(resultRaw)) {
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

size_t fuzzyFactor(PinyinFuzzyFlags flags) {
    size_t factor = 0;
    if (flags.test(PinyinFuzzyFlag::Correction)) {
        flags = flags.unset(PinyinFuzzyFlag::Correction);
        factor += PINYIN_CORRECTION_FUZZY_FACTOR;
    }
    if (flags.test(PinyinFuzzyFlag::AdvancedTypo)) {
        flags = flags.unset(PinyinFuzzyFlag::AdvancedTypo);
        factor += PINYIN_ADVACNED_TYPO_FUZZY_FACTOR;
    }
    if (flags != 0) {
        factor += 1;
    }
    return factor;
}

PinyinDictionary::TrieType loadTextImpl(std::istream &in) {
    PinyinDictionary::TrieType trie;

    std::string buf;
    auto isSpaceCheck = boost::is_any_of(" \n\t\r\v\f");
    while (!in.eof()) {
        if (!std::getline(in, buf)) {
            break;
        }

        boost::trim_if(buf, isSpaceCheck);
        std::vector<std::string> tokens;
        boost::split(tokens, buf, isSpaceCheck);
        if (tokens.size() == 3 || tokens.size() == 2) {
            const std::string &hanzi = tokens[0];
            std::string_view pinyin = tokens[1];
            float prob = 0.0F;
            if (tokens.size() == 3) {
                prob = std::stof(tokens[2]);
            }

            try {
                auto result = PinyinEncoder::encodeFullPinyinWithFlags(
                    pinyin, PinyinFuzzyFlag::VE_UE);
                result.push_back(pinyinHanziSep);
                result.insert(result.end(), hanzi.begin(), hanzi.end());
                trie.set(result.data(), result.size(), prob);
            } catch (const std::invalid_argument &e) {
                LIBIME_ERROR()
                    << "Failed to parse line: " << buf << ", skipping.";
            }
        }
    }
    return trie;
}

PinyinDictionary::TrieType loadBinaryImpl(std::istream &in) {
    PinyinDictionary::TrieType trie;
    uint32_t magic = 0;
    uint32_t version = 0;
    throw_if_io_fail(unmarshall(in, magic));
    if (magic != pinyinBinaryFormatMagic) {
        throw std::invalid_argument("Invalid pinyin magic.");
    }
    throw_if_io_fail(unmarshall(in, version));
    switch (version) {
    case 0x1:
        trie.load(in);
        break;
    case pinyinBinaryFormatVersion:
        readZSTDCompressed(
            in, [&trie](std::istream &compressIn) { trie.load(compressIn); });
        break;
    default:
        throw std::invalid_argument("Invalid pinyin version.");
        break;
    }
    return trie;
}

} // namespace

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
          spProfile_(matchState->shuangpinProfile()),
          correctionProfile_(matchState->correctionProfile()),
          partialLongWordLimit_(matchState->partialLongWordLimit()) {}

    explicit PinyinMatchContext(
        const SegmentGraph &graph, const GraphMatchCallback &callback,
        const std::unordered_set<const SegmentGraphNode *> &ignore,
        NodeToMatchedPinyinPathsMap &matchedPaths)
        : graph_(graph), hasher_(graph), callback_(callback), ignore_(ignore),
          matchedPathsMap_(&matchedPaths), nodeCacheMap_(nullptr),
          matchCacheMap_(nullptr) {}

    PinyinMatchContext(const PinyinMatchContext &) = delete;

    const SegmentGraph &graph_;
    PinyinSegmentGraphPathHasher hasher_;

    const GraphMatchCallback &callback_;
    const std::unordered_set<const SegmentGraphNode *> &ignore_;
    NodeToMatchedPinyinPathsMap *matchedPathsMap_;
    PinyinTrieNodeCache *nodeCacheMap_ = nullptr;
    PinyinMatchResultCache *matchCacheMap_ = nullptr;
    PinyinFuzzyFlags flags_{PinyinFuzzyFlag::None};
    std::shared_ptr<const ShuangpinProfile> spProfile_;
    std::shared_ptr<const PinyinCorrectionProfile> correctionProfile_;
    size_t partialLongWordLimit_ = 0;
};

class PinyinDictionaryPrivate : fcitx::QPtrHolder<PinyinDictionary> {
public:
    PinyinDictionaryPrivate(PinyinDictionary *q)
        : fcitx::QPtrHolder<PinyinDictionary>(q) {}

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

    fcitx::ScopedConnection conn_;
    std::vector<PinyinDictFlags> flags_;
};

void PinyinDictionaryPrivate::addEmptyMatch(
    const PinyinMatchContext &context, const SegmentGraphNode &currentNode,
    MatchedPinyinPaths &currentMatches) const {
    FCITX_Q();
    const SegmentGraph &graph = context.graph_;
    // Create a new starting point for current node, and put it in matchResult.
    if (&currentNode != &graph.end() &&
        !boost::starts_with(
            graph.segment(currentNode.index(), currentNode.index() + 1),
            "\'")) {
        SegmentGraphPath vec;
        if (const auto *prev = prevIsSeparator(graph, currentNode)) {
            vec.push_back(prev);
        }

        vec.push_back(&currentNode);
        for (size_t i = 0; i < q->dictSize(); i++) {
            if (flags_[i].test(PinyinDictFlag::FullMatch) &&
                &currentNode != &graph.start()) {
                continue;
            }
            if (flags_[i].test(PinyinDictFlag::Disabled)) {
                continue;
            }
            const auto &trie = *q->trie(i);
            currentMatches.emplace_back(&trie, 0, vec, flags_[i]);
            currentMatches.back().triePositions().emplace_back(0, 0);
        }
    }
}

PinyinTriePositions traverseAlongPathOneStepBySyllables(
    const MatchedPinyinPath &path,
    const MatchedPinyinSyllablesWithFuzzyFlags &syls) {
    PinyinTriePositions positions;
    for (const auto &pr : path.triePositions()) {
        uint64_t _pos;
        size_t fuzzies;
        std::tie(_pos, fuzzies) = pr;
        for (const auto &syl : syls) {
            // make a copy
            auto pos = _pos;
            auto initial = static_cast<char>(syl.first);
            const auto resultRaw = path.trie()->traverseRaw(&initial, 1, pos);
            if (PinyinTrie::isNoPathRaw(resultRaw)) {
                continue;
            }
            const auto &finals = syl.second;

            auto updateNext = [fuzzies, &path, &positions](PinyinFinal pyFinal,
                                                           size_t fuzzyFactor,
                                                           auto pos) {
                auto final = static_cast<char>(pyFinal);
                const auto resultRaw = path.trie()->traverseRaw(&final, 1, pos);

                if (!PinyinTrie::isNoPathRaw(resultRaw)) {
                    size_t newFuzzies = fuzzies + fuzzyFactor;
                    positions.emplace_back(pos, newFuzzies);
                }
            };
            if (finals.size() > 1 || finals[0].first != PinyinFinal::Invalid) {
                for (auto final : finals) {
                    updateNext(final.first, fuzzyFactor(final.second), pos);
                }
            } else if (!path.flags_.test(PinyinDictFlag::FullMatch)) {
                for (char test = PinyinEncoder::firstFinal;
                     test <= PinyinEncoder::lastFinal; test++) {
                    updateNext(static_cast<PinyinFinal>(test), 1, pos);
                }
            }
        }
    }
    return positions;
}

template <typename T>
void matchWordsOnTrie(const PinyinTrie *userDict, const MatchedPinyinPath &path,
                      bool matchLongWord, const T &callback) {
    for (const auto &pr : path.triePositions()) {
        uint64_t pos;
        size_t fuzzies;
        std::tie(pos, fuzzies) = pr;
        const float extraCost = fuzzies * fuzzyCost;
        // This is an inaccuration estimation, since fuzzies may contain real
        // fuzzy pinyin. But since this value is 10, it is a good estimate.
        // After all 10 fuzzies in a word is kinda impossible.
        const bool isCorrection = fuzzies >= PINYIN_CORRECTION_FUZZY_FACTOR;
        if (matchLongWord) {
            path.trie()->foreach(
                [userDict, &path, &callback, extraCost, isCorrection](
                    PinyinTrie::value_type value, size_t len, uint64_t pos) {
                    std::string s;
                    s.reserve(len + (path.size() * 2));
                    path.trie()->suffix(s, len + (path.size() * 2), pos);
                    if (size_t separator =
                            s.find(pinyinHanziSep, path.size() * 2);
                        separator != std::string::npos) {
                        std::string_view view(s);
                        auto encodedPinyin = view.substr(0, separator);
                        auto hanzi = view.substr(separator + 1);
                        const size_t lengthDiff =
                            ((encodedPinyin.size() / 2) - path.size());
                        // Don't match long word for "custom".
                        if (path.trie() == userDict && value < 0 &&
                            lengthDiff > 0) {
                            return true;
                        }
                        float overLengthCost = fuzzyCost * lengthDiff;

                        callback(encodedPinyin, hanzi,
                                 value + extraCost + overLengthCost,
                                 isCorrection);
                    }
                    return true;
                },
                pos);
        } else {
            const char sep = pinyinHanziSep;
            const auto resultRaw = path.trie()->traverseRaw(&sep, 1, pos);
            if (PinyinTrie::isNoPathRaw(resultRaw)) {
                continue;
            }

            path.trie()->foreach(
                [&path, &callback, extraCost, isCorrection](
                    PinyinTrie::value_type value, size_t len, uint64_t pos) {
                    std::string s;
                    s.reserve(len + (path.size() * 2) + 1);
                    path.trie()->suffix(s, len + (path.size() * 2) + 1, pos);
                    std::string_view view(s);
                    auto encodedPinyin = view.substr(0, path.size() * 2);
                    auto hanzi = view.substr((path.size() * 2) + 1);
                    callback(encodedPinyin, hanzi, value + extraCost,
                             isCorrection);
                    return true;
                },
                pos);
        }
    }
}

bool PinyinDictionaryPrivate::matchWordsForOnePath(
    const PinyinMatchContext &context, const MatchedPinyinPath &path) const {
    FCITX_Q();
    bool matched = false;
    assert(path.path_.size() >= 2);
    const SegmentGraphNode &prevNode = *path.path_[path.path_.size() - 2];

    if (path.flags_.test(PinyinDictFlag::FullMatch) &&
        (path.path_.front() != &context.graph_.start() ||
         path.path_.back() != &context.graph_.end())) {
        return false;
    }

    // minimumLongWordLength is to prevent algorithm runs too slow.
    const bool matchLongWordEnabled =
        context.partialLongWordLimit_ &&
        std::max(minimumLongWordLength, context.partialLongWordLimit_) + 1 <=
            path.path_.size() &&
        !path.flags_.test(PinyinDictFlag::FullMatch);

    const bool matchLongWord =
        (path.path_.back() == &context.graph_.end() && matchLongWordEnabled);

    auto foundOneWord = [&path, &prevNode, &matched, &context](
                            std::string_view encodedPinyin, WordNode &word,
                            float cost, bool isCorrection) {
        context.callback_(path.path_, word, cost,
                          std::make_unique<PinyinLatticeNodePrivate>(
                              encodedPinyin, isCorrection));
        if (path.size() == 1 &&
            path.path_[path.path_.size() - 2] == &prevNode) {
            matched = true;
        }
    };

    if (context.matchCacheMap_) {
        auto &matchCache = (*context.matchCacheMap_)[path.trie()];
        auto *result =
            matchCache.find(path.path_, context.hasher_, context.hasher_);
        if (!result) {
            result =
                matchCache.insert(context.hasher_.pathToPinyins(path.path_));
            result->clear();

            auto &items = *result;
            matchWordsOnTrie(
                q->trie(PinyinDictionary::UserDict), path, matchLongWordEnabled,
                [&items](std::string_view encodedPinyin, std::string_view hanzi,
                         float cost, bool isCorrection) {
                    items.emplace_back(hanzi, cost, encodedPinyin,
                                       isCorrection);
                });
        }
        for (auto &item : *result) {
            if (!matchLongWord &&
                item.encodedPinyin_.size() / 2 > path.size()) {
                continue;
            }
            foundOneWord(item.encodedPinyin_, item.word_, item.value_,
                         item.isCorrection_);
        }
    } else {
        matchWordsOnTrie(
            q->trie(PinyinDictionary::UserDict), path, matchLongWord,
            [&foundOneWord](std::string_view encodedPinyin,
                            std::string_view hanzi, float cost,
                            bool isCorrection) {
                WordNode word(hanzi, InvalidWordIndex);
                foundOneWord(encodedPinyin, word, cost, isCorrection);
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
        for (const auto &match : prevMatches) {
            // copy the path, and append current node.
            auto path = match.path_;
            path.push_back(&currentNode);
            currentMatches.emplace_back(match.result_, std::move(path),
                                        match.flags_);
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
            ? PinyinEncoder::shuangpinToSyllablesWithFuzzyFlags(
                  pinyin, *context.spProfile_, context.flags_)
            : PinyinEncoder::stringToSyllablesWithFuzzyFlags(
                  pinyin, context.correctionProfile_.get(), context.flags_);
    const MatchedPinyinPaths &prevMatchedPaths = matchedPathsMap[&prevNode];
    MatchedPinyinPaths newPaths;
    for (const auto &path : prevMatchedPaths) {
        // Make a copy of path so we can modify based on it.
        auto segmentPath = path.path_;
        segmentPath.push_back(&currentNode);

        // A map from trie (dict) to a lru cache.
        if (context.nodeCacheMap_) {
            auto &nodeCache = (*context.nodeCacheMap_)[path.trie()];
            auto *p =
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

            if (!result->triePositions_.empty()) {
                newPaths.emplace_back(result, segmentPath, path.flags_);
            }
        } else {
            // make an empty one
            newPaths.emplace_back(path.trie(), path.size() + 1, segmentPath,
                                  path.flags_);

            newPaths.back().result_->triePositions_ =
                traverseAlongPathOneStepBySyllables(path, syls);
            // if there's nothing, pop it.
            if (newPaths.back().triePositions().empty()) {
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
            if (const auto *prevPrev =
                    prevIsSeparator(context.graph_, prevNode)) {
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
    for (const auto &prevNode : currentNode.prevs()) {
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

    const auto &start = graph.start();
    q.push(&start);

    // The match is done with a bfs.
    // E.g
    // xian is
    // start - xi - an - end
    //       \           /
    //        -- xian ---
    // We start with start, then xi, then an and xian, then end.
    while (!q.empty()) {
        const auto *currentNode = q.top();
        q.pop();

        // Push successors into the queue.
        for (const auto &node : currentNode->nexts()) {
            q.push(&node);
        }

        d->matchNode(context, *currentNode);
    }
}

void PinyinDictionary::matchWords(const char *data, size_t size,
                                  PinyinMatchCallback callback) const {
    if (!PinyinEncoder::isValidUserPinyin(data, size)) {
        return;
    }

    FCITX_D();
    std::list<std::pair<const PinyinTrie *, PinyinTrie::position_type>> nodes;
    for (size_t i = 0; i < dictSize(); i++) {
        if (d->flags_[i].test(PinyinDictFlag::Disabled)) {
            continue;
        }
        const auto &trie = *this->trie(i);
        nodes.emplace_back(&trie, 0);
    }
    for (size_t i = 0; i <= size && !nodes.empty(); i++) {
        char current;
        if (i < size) {
            current = data[i];
        } else {
            current = pinyinHanziSep;
        }
        searchOneStep(nodes, current);
    }

    for (auto &node : nodes) {
        node.first->foreach(
            [&node, &callback, size](PinyinTrie::value_type value, size_t len,
                                     uint64_t pos) {
                std::string s;
                node.first->suffix(s, len + size + 1, pos);

                auto view = std::string_view(s);
                return callback(view.substr(0, size), view.substr(size + 1),
                                value);
            },
            node.second);
    }
}

void PinyinDictionary::matchWordsPrefix(const char *data, size_t size,
                                        PinyinMatchCallback callback) const {
    if (!PinyinEncoder::isValidUserPinyin(data, size)) {
        return;
    }

    FCITX_D();
    std::list<std::pair<const PinyinTrie *, PinyinTrie::position_type>> nodes;
    for (size_t i = 0; i < dictSize(); i++) {
        if (d->flags_[i].test(PinyinDictFlag::Disabled)) {
            continue;
        }
        const auto &trie = *this->trie(i);
        nodes.emplace_back(&trie, 0);
    }
    for (size_t i = 0; i < size && !nodes.empty(); i++) {
        searchOneStep(nodes, data[i]);
    }

    for (auto &node : nodes) {
        node.first->foreach(
            [&node, &callback, size](PinyinTrie::value_type value, size_t len,
                                     uint64_t pos) {
                std::string s;
                node.first->suffix(s, len + size, pos);

                std::string_view view(s);
                if (auto sep = view.find(pinyinHanziSep, size);
                    sep != std::string::npos) {
                    return callback(view.substr(0, sep), view.substr(sep + 1),
                                    value);
                }
                return true;
            },
            node.second);
    }
}

PinyinDictionary::PinyinDictionary()
    : d_ptr(std::make_unique<PinyinDictionaryPrivate>(this)) {
    FCITX_D();
    d->conn_ = connect<TrieDictionary::dictSizeChanged>([this](size_t size) {
        FCITX_D();
        d->flags_.resize(size);
    });
    d->flags_.resize(dictSize());
}

PinyinDictionary::~PinyinDictionary() {}

void PinyinDictionary::load(size_t idx, const char *filename,
                            PinyinDictFormat format) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    throw_if_io_fail(in);
    load(idx, in, format);
}

void PinyinDictionary::load(size_t idx, std::istream &in,
                            PinyinDictFormat format) {
    setTrie(idx, load(in, format));
}

PinyinDictionary::TrieType PinyinDictionary::load(std::istream &in,
                                                  PinyinDictFormat format) {
    switch (format) {
    case PinyinDictFormat::Text:
        return loadTextImpl(in);
    case PinyinDictFormat::Binary:
        return loadBinaryImpl(in);
    default:
        throw std::invalid_argument("invalid format type");
    }
}

void PinyinDictionary::loadText(size_t idx, std::istream &in) {
    *mutableTrie(idx) = loadTextImpl(in);
}

void PinyinDictionary::loadBinary(size_t idx, std::istream &in) {
    *mutableTrie(idx) = loadBinaryImpl(in);
}

void PinyinDictionary::save(size_t idx, const char *filename,
                            PinyinDictFormat format) {
    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    throw_if_io_fail(fout);
    save(idx, fout, format);
}

void PinyinDictionary::save(size_t idx, std::ostream &out,
                            PinyinDictFormat format) {
    switch (format) {
    case PinyinDictFormat::Text:
        saveText(idx, out);
        break;
    case PinyinDictFormat::Binary: {
        throw_if_io_fail(marshall(out, pinyinBinaryFormatMagic));
        throw_if_io_fail(marshall(out, pinyinBinaryFormatVersion));

        writeZSTDCompressed(out, [this, idx](std::ostream &compressOut) {
            mutableTrie(idx)->save(compressOut);
        });
    } break;
    default:
        throw std::invalid_argument("invalid format type");
    }
}

void PinyinDictionary::saveText(size_t idx, std::ostream &out) {
    std::string buf;
    std::ios state(nullptr);
    state.copyfmt(out);
    const auto &trie = *this->trie(idx);
    trie.foreach([&trie, &buf, &out](float value, size_t _len,
                                     PinyinTrie::position_type pos) {
        trie.suffix(buf, _len, pos);
        auto sep = buf.find(pinyinHanziSep);
        if (sep == std::string::npos) {
            return true;
        }
        std::string_view ref(buf);
        auto fullPinyin = PinyinEncoder::decodeFullPinyin(ref.data(), sep);
        out << ref.substr(sep + 1) << " " << fullPinyin << " "
            << std::setprecision(16) << value << '\n';
        return true;
    });
    out.copyfmt(state);
}

void PinyinDictionary::addWord(size_t idx, std::string_view fullPinyin,
                               std::string_view hanzi, float cost) {
    auto result = PinyinEncoder::encodeFullPinyinWithFlags(
        fullPinyin, PinyinFuzzyFlag::VE_UE);
    result.push_back(pinyinHanziSep);
    result.insert(result.end(), hanzi.begin(), hanzi.end());
    TrieDictionary::addWord(idx, std::string_view(result.data(), result.size()),
                            cost);
}

std::optional<float>
PinyinDictionary::lookupWord(size_t idx, std::string_view fullPinyin,
                             std::string_view hanzi) const {
    auto result = PinyinEncoder::encodeFullPinyinWithFlags(
        fullPinyin, PinyinFuzzyFlag::VE_UE);
    result.push_back(pinyinHanziSep);
    result.insert(result.end(), hanzi.begin(), hanzi.end());
    auto value = trie(idx)->exactMatchSearch(
        std::string_view(result.data(), result.size()));
    if (PinyinTrie::isValid(value)) {
        return value;
    }
    return std::nullopt;
}

bool PinyinDictionary::removeWord(size_t idx, std::string_view fullPinyin,
                                  std::string_view hanzi) {
    auto result = PinyinEncoder::encodeFullPinyinWithFlags(
        fullPinyin, PinyinFuzzyFlag::VE_UE);
    result.push_back(pinyinHanziSep);
    result.insert(result.end(), hanzi.begin(), hanzi.end());
    return TrieDictionary::removeWord(
        idx, std::string_view(result.data(), result.size()));
}

void PinyinDictionary::setFlags(size_t idx, PinyinDictFlags flags) {
    FCITX_D();
    if (idx >= dictSize()) {
        return;
    }
    d->flags_.resize(dictSize());
    d->flags_[idx] = flags;
}
} // namespace libime
