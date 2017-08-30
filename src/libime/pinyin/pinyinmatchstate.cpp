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

#include "pinyincontext.h"
#include "pinyinime.h"
#include "pinyinmatchstate_p.h"

namespace libime {

PinyinMatchState::PinyinMatchState(PinyinContext *context)
    : d_ptr(std::make_unique<PinyinMatchStatePrivate>(context)) {}
PinyinMatchState::~PinyinMatchState() {}

void PinyinMatchState::clear() {
    FCITX_D();
    d->matchedPaths_.clear();
    d->nodeCacheMap_.clear();
    d->matchCacheMap_.clear();
}

void PinyinMatchState::discardNode(
    const std::unordered_set<const SegmentGraphNode *> &nodes) {
    FCITX_D();
    for (auto node : nodes) {
        d->matchedPaths_.erase(node);
    }
    for (auto &p : d->matchedPaths_) {
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

PinyinFuzzyFlags PinyinMatchState::fuzzyFlags() const {
    FCITX_D();
    return d->context_->ime()->fuzzyFlags();
}

std::shared_ptr<const ShuangpinProfile>
PinyinMatchState::shuangpinProfile() const {
    FCITX_D();
    if (d->context_->useShuangpin()) {
        return d->context_->ime()->shuangpinProfile();
    }
    return {};
}

void PinyinMatchState::discardDictionary(size_t idx) {
    FCITX_D();
    d->matchCacheMap_.erase(d->context_->ime()->dict()->trie(idx));
    d->nodeCacheMap_.erase(d->context_->ime()->dict()->trie(idx));
}
}
