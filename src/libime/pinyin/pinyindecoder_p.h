/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_PINYIN_PINYINDECODER_P_H_
#define _FCITX_LIBIME_PINYIN_PINYINDECODER_P_H_

#include "pinyindecoder.h"
#include <libime/core/lattice.h>

namespace libime {

class PinyinLatticeNodePrivate : public LatticeNodeData {
public:
    PinyinLatticeNodePrivate(std::string_view encodedPinyin, bool isCorrection)
        : encodedPinyin_(encodedPinyin), isCorrection_(isCorrection) {}

    std::string encodedPinyin_;
    bool isCorrection_ = false;
};
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINDECODER_P_H_
