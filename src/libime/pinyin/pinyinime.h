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
#ifndef _FCITX_LIBIME_PINYIN_PINYINIME_H_
#define _FCITX_LIBIME_PINYIN_PINYINIME_H_

#include "libimepinyin_export.h"
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/macros.h>
#include <libime/pinyin/pinyinencoder.h>
#include <limits>
#include <memory>

namespace libime {

class PinyinIMEPrivate;
class PinyinDecoder;
class PinyinDictionary;
class UserLanguageModel;

class LIBIMEPINYIN_EXPORT PinyinIME : public fcitx::ConnectableObject {
public:
    PinyinIME(std::unique_ptr<PinyinDictionary> dict,
              std::unique_ptr<UserLanguageModel> model);
    virtual ~PinyinIME();

    PinyinFuzzyFlags fuzzyFlags() const;
    void setFuzzyFlags(PinyinFuzzyFlags flags);
    size_t nbest() const;
    void setNBest(size_t n);
    size_t beamSize() const;
    void setBeamSize(size_t n);
    size_t frameSize() const;
    void setFrameSize(size_t n);
    void setScoreFilter(float maxDistance = std::numeric_limits<float>::max(),
                        float minPath = -std::numeric_limits<float>::max());
    void setShuangpinProfile(std::shared_ptr<const ShuangpinProfile> profile);
    std::shared_ptr<const ShuangpinProfile> shuangpinProfile() const;

    float maxDistance() const;
    float minPath() const;

    PinyinDictionary *dict();
    const PinyinDictionary *dict() const;
    const PinyinDecoder *decoder() const;
    UserLanguageModel *model();
    const UserLanguageModel *model() const;

    FCITX_DECLARE_SIGNAL(PinyinIME, optionChanged, void());

private:
    std::unique_ptr<PinyinIMEPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(PinyinIME);
};
}

#endif // _FCITX_LIBIME_PINYIN_PINYINIME_H_
