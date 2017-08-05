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
#ifndef _FCITX_LIBIME_PINYINMATCHSTATE_P_H_
#define _FCITX_LIBIME_PINYINMATCHSTATE_P_H_

#include "lattice.h"
#include "lrucache.h"
#include "pinyindictionary.h"
#include "pinyinmatchstate.h"
#include <fcitx-utils/macros.h>
#include <memory>
#include <unordered_map>

namespace libime {

using PinyinTriePosition = std::pair<uint64_t, size_t>;
using PinyinTriePositions = std::vector<PinyinTriePosition>;

// Matching result for a specific PinyinTrie.
struct MatchedPinyinTrieNodes {
    MatchedPinyinTrieNodes(const PinyinTrie *trie, size_t size)
        : trie_(trie), size_(size) {}
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(MatchedPinyinTrieNodes)

    const PinyinTrie *trie_;
    PinyinTriePositions triePositions_;

    // Size of syllables.
    size_t size_;
};

// A cache to store the matched word, encoded Full Pinyin for this word and the
// adjustment score.
struct PinyinMatchResult {
    PinyinMatchResult(boost::string_view s, float value,
                      boost::string_view encodedPinyin)
        : word_(s, InvalidWordIndex), value_(value),
          encodedPinyin_(encodedPinyin.to_string()) {}
    WordNode word_;
    float value_;
    std::string encodedPinyin_;
};

// class to store current SegmentGraphPath leads to this match and the match
// reuslt.
struct MatchedPinyinPath {
    MatchedPinyinPath(const PinyinTrie *trie, size_t size,
                      SegmentGraphPath path)
        : result_(std::make_shared<MatchedPinyinTrieNodes>(trie, size)),
          path_(std::move(path)) {}

    MatchedPinyinPath(std::shared_ptr<MatchedPinyinTrieNodes> result,
                      SegmentGraphPath path)
        : result_(result), path_(std::move(path)) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(MatchedPinyinPath)

    auto &triePositions() { return result_->triePositions_; }
    const auto &triePositions() const { return result_->triePositions_; }
    const PinyinTrie *trie() const { return result_->trie_; }

    // Size of syllables. not necessarily equal to size of path_, because there
    // may be separators.
    auto size() const { return result_->size_; }

    std::shared_ptr<MatchedPinyinTrieNodes> result_;
    SegmentGraphPath path_;
};

// A list of all search paths
typedef std::vector<MatchedPinyinPath> MatchedPinyinPaths;

// Map from SegmentGraphNode to Search Paths.
typedef std::unordered_map<const SegmentGraphNode *, MatchedPinyinPaths>
    NodeToMatchedPinyinPathsMap;

// A cache for all PinyinTries. From a pinyin string to its matched
// PinyinTrieNode
typedef std::unordered_map<
    const PinyinTrie *,
    LRUCache<std::string, std::shared_ptr<MatchedPinyinTrieNodes>>>
    PinyinTrieNodeCache;

// A cache for PinyinMatchResult.
typedef std::unordered_map<
    const PinyinTrie *, LRUCache<std::string, std::vector<PinyinMatchResult>>>
    PinyinMatchResultCache;

class PinyinMatchStatePrivate {
public:
    PinyinMatchStatePrivate(PinyinContext *context) : context_(context) {}

    PinyinContext *context_;
    NodeToMatchedPinyinPathsMap matchedPaths_;
    PinyinTrieNodeCache nodeCacheMap_;
    PinyinMatchResultCache matchCacheMap_;
};
}

#endif // _FCITX_LIBIME_PINYINMATCHSTATE_P_H_
