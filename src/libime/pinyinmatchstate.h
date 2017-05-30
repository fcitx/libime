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
#ifndef _FCITX_LIBIME_PINYINMATCHSTATE_H_
#define _FCITX_LIBIME_PINYINMATCHSTATE_H_

#include "libime_export.h"
#include "pinyinencoder.h"
#include <fcitx-utils/macros.h>
#include <unordered_set>

namespace libime {

class PinyinMatchStatePrivate;
class SegmentGraphNode;
class ShuangpinProfile;
class PinyinContext;

class LIBIME_EXPORT PinyinMatchState {
    friend class PinyinDictionary;

public:
    PinyinMatchState(PinyinContext *context);
    ~PinyinMatchState();

    void clear();
    void discardNode(const std::unordered_set<const SegmentGraphNode *> &node);
    void discardDictionary(size_t idx);

    PinyinFuzzyFlags fuzzyFlags() const;
    std::shared_ptr<const ShuangpinProfile> shuangpinProfile() const;

private:
    std::unique_ptr<PinyinMatchStatePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(PinyinMatchState);
};
}

#endif // _FCITX_LIBIME_PINYINMATCHSTATE_H_
