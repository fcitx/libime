/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/pinyin/pinyinmatchstate.h"
#include "libime/pinyin/pinyinencoder.h"
#include "pinyincontext.h"
#include "pinyinime.h"
#include "pinyinmatchstate_p.h"
#include <cstddef>
#include <fcitx-utils/macros.h>
#include <memory>
#include <unordered_set>

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
    for (const auto *node : nodes) {
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

std::shared_ptr<const PinyinCorrectionProfile>
PinyinMatchState::correctionProfile() const {
    FCITX_D();
    if (d->context_->ime()->fuzzyFlags().test(PinyinFuzzyFlag::Correction)) {
        return d->context_->ime()->correctionProfile();
    }
    return {};
}

size_t PinyinMatchState::partialLongWordLimit() const {
    FCITX_D();
    return d->context_->ime()->partialLongWordLimit();
}

void PinyinMatchState::discardDictionary(size_t idx) {
    FCITX_D();
    d->matchCacheMap_.erase(d->context_->ime()->dict()->trie(idx));
    d->nodeCacheMap_.erase(d->context_->ime()->dict()->trie(idx));
}
} // namespace libime
