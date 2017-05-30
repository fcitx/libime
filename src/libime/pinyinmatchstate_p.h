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
#include <memory>
#include <unordered_map>

namespace libime {

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

typedef std::unordered_map<const PinyinTrie *,
                           LRUCache<std::string, std::shared_ptr<MatchResult>>>
    MatchNodeCache;
typedef std::unordered_map<const PinyinTrie *,
                           LRUCache<std::string, std::vector<MatchItem>>>
    MatchCache;

class PinyinMatchStatePrivate {
public:
    PinyinMatchStatePrivate(PinyinContext *context) : context_(context) {}

    PinyinContext *context_;
    MatchStateMap search_;
    MatchNodeCache nodeCache_;
    MatchCache matchCache_;
};
}

#endif // _FCITX_LIBIME_PINYINMATCHSTATE_P_H_
