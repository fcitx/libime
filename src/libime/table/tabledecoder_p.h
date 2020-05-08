/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_TABLE_TABLEDECODER_P_H_
#define _FCITX_LIBIME_TABLE_TABLEDECODER_P_H_

#include "tabledecoder.h"
#include <string>

namespace libime {

class TableLatticeNodePrivate : public LatticeNodeData {
public:
    TableLatticeNodePrivate(std::string_view code, uint32_t index,
                            PhraseFlag flag)
        : code_(code), index_(index), flag_(flag) {}

    std::string code_;
    uint32_t index_;
    PhraseFlag flag_;
};
} // namespace libime

#endif // _FCITX_LIBIME_TABLE_TABLEDECODER_P_H_
