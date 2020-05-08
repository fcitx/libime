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
    PinyinLatticeNodePrivate(std::string_view encodedPinyin)
        : encodedPinyin_(encodedPinyin) {}

    std::string encodedPinyin_;
};
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINDECODER_P_H_
