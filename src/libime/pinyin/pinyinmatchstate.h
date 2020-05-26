/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_PINYIN_PINYINMATCHSTATE_H_
#define _FCITX_LIBIME_PINYIN_PINYINMATCHSTATE_H_

#include "libimepinyin_export.h"
#include <fcitx-utils/macros.h>
#include <libime/pinyin/pinyinencoder.h>
#include <unordered_set>

namespace libime {

class PinyinMatchStatePrivate;
class SegmentGraphNode;
class ShuangpinProfile;
class PinyinContext;

// Provides caching mechanism used by PinyinContext.
class LIBIMEPINYIN_EXPORT PinyinMatchState {
    friend class PinyinMatchContext;

public:
    PinyinMatchState(PinyinContext *context);
    ~PinyinMatchState();

    // Invalidate everything in the state.
    void clear();

    // Invalidate a set of node, usually caused by the change of user input.
    void discardNode(const std::unordered_set<const SegmentGraphNode *> &node);

    // Invalidate a whole dictionary, usually caused by the change to the
    // dictionary.
    void discardDictionary(size_t idx);

    PinyinFuzzyFlags fuzzyFlags() const;
    std::shared_ptr<const ShuangpinProfile> shuangpinProfile() const;
    size_t partialLongWordLimit() const;

private:
    std::unique_ptr<PinyinMatchStatePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(PinyinMatchState);
};
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINMATCHSTATE_H_
