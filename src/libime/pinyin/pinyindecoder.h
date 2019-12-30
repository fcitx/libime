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
#ifndef _FCITX_LIBIME_PINYIN_PINYINDECODER_H_
#define _FCITX_LIBIME_PINYIN_PINYINDECODER_H_

#include "libimepinyin_export.h"
#include <libime/core/decoder.h>
#include <libime/pinyin/pinyindictionary.h>

namespace libime {

class PinyinLatticeNodePrivate;

class LIBIMEPINYIN_EXPORT PinyinLatticeNode : public LatticeNode {
public:
    PinyinLatticeNode(std::string_view word, WordIndex idx,
                      SegmentGraphPath path, const State &state, float cost,
                      std::unique_ptr<PinyinLatticeNodePrivate> data);
    virtual ~PinyinLatticeNode();

    const std::string &encodedPinyin() const;

private:
    std::unique_ptr<PinyinLatticeNodePrivate> d_ptr;
};

class LIBIMEPINYIN_EXPORT PinyinDecoder : public Decoder {
public:
    PinyinDecoder(const PinyinDictionary *dict, const LanguageModelBase *model)
        : Decoder(dict, model) {}

protected:
    LatticeNode *createLatticeNodeImpl(const SegmentGraphBase &graph,
                                       const LanguageModelBase *model,
                                       std::string_view word, WordIndex idx,
                                       SegmentGraphPath path,
                                       const State &state, float cost,
                                       std::unique_ptr<LatticeNodeData> data,
                                       bool onlyPath) const override;
};
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINDECODER_H_
