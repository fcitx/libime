/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_PINYIN_PINYINMATCHSTATE_P_H_
#define _FCITX_LIBIME_PINYIN_PINYINMATCHSTATE_P_H_

#include "libime/core/languagemodel.h"
#include "libime/core/segmentgraph.h"
#include <boost/container_hash/hash.hpp>
#include <cstddef>
#include <cstdint>
#include <fcitx-utils/macros.h>
#include <libime/core/lattice.h>
#include <libime/core/lrucache.h>
#include <libime/pinyin/pinyindictionary.h>
#include <libime/pinyin/pinyinmatchstate.h>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

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
    PinyinMatchResult(std::string_view s, float value,
                      std::string_view encodedPinyin, bool isCorrection)
        : word_(s, InvalidWordIndex), value_(value),
          encodedPinyin_(encodedPinyin), isCorrection_(isCorrection) {}
    WordNode word_;
    float value_ = 0.0F;
    std::string encodedPinyin_;
    bool isCorrection_ = false;
};

// class to store current SegmentGraphPath leads to this match and the match
// reuslt.
struct MatchedPinyinPath {
    MatchedPinyinPath(const PinyinTrie *trie, size_t size,
                      SegmentGraphPath path, PinyinDictFlags flags)
        : result_(std::make_shared<MatchedPinyinTrieNodes>(trie, size)),
          path_(std::move(path)), flags_(flags) {}

    MatchedPinyinPath(std::shared_ptr<MatchedPinyinTrieNodes> result,
                      SegmentGraphPath path, PinyinDictFlags flags)
        : result_(std::move(result)), path_(std::move(path)), flags_(flags) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(MatchedPinyinPath)

    auto &triePositions() { return result_->triePositions_; }
    const auto &triePositions() const { return result_->triePositions_; }
    const PinyinTrie *trie() const { return result_->trie_; }

    // Size of syllables. not necessarily equal to size of path_, because there
    // may be separators.
    auto size() const { return result_->size_; }

    std::shared_ptr<MatchedPinyinTrieNodes> result_;
    SegmentGraphPath path_;
    PinyinDictFlags flags_;
};

// This need to be keep sync with PinyinSegmentGraphPathHasher
class PinyinStringHasher {
public:
    size_t operator()(const std::string &s) const {
        boost::hash<char> hasher;

        size_t seed = 0;
        for (char c : s) {
            boost::hash_combine(seed, hasher(c));
        }
        return seed;
    }
};

// A list of all search paths
using MatchedPinyinPaths = std::vector<MatchedPinyinPath>;

// Map from SegmentGraphNode to Search Paths.
using NodeToMatchedPinyinPathsMap =
    std::unordered_map<const SegmentGraphNode *, MatchedPinyinPaths>;

// A cache for all PinyinTries. From a pinyin string to its matched
// PinyinTrieNode
using PinyinTrieNodeCache = std::unordered_map<
    const PinyinTrie *,
    LRUCache<std::string, std::shared_ptr<MatchedPinyinTrieNodes>,
             PinyinStringHasher>>;

// A cache for PinyinMatchResult.
using PinyinMatchResultCache = std::unordered_map<
    const PinyinTrie *,
    LRUCache<std::string, std::vector<PinyinMatchResult>, PinyinStringHasher>>;

class PinyinMatchStatePrivate {
public:
    PinyinMatchStatePrivate(PinyinContext *context) : context_(context) {}

    PinyinContext *context_;
    NodeToMatchedPinyinPathsMap matchedPaths_;
    PinyinTrieNodeCache nodeCacheMap_;
    PinyinMatchResultCache matchCacheMap_;
};
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINMATCHSTATE_P_H_
