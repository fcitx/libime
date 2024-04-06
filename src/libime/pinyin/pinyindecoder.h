/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
    bool isCorrection() const;
    bool anyCorrectionOnPath() const;

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
