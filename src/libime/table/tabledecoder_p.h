/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_TABLE_TABLEDECODER_P_H_
#define _FCITX_LIBIME_TABLE_TABLEDECODER_P_H_

#include "libime/core/lattice.h"
#include "libime/table/tablebaseddictionary.h"
#include <cstddef>
#include <cstdint>
#include <fcitx-utils/utf8.h>
#include <string>
#include <string_view>

namespace libime {

class TableLatticeNodePrivate : public LatticeNodeData {
public:
    TableLatticeNodePrivate(std::string_view code, uint32_t index,
                            PhraseFlag flag)
        : code_(code), codeLength_(fcitx::utf8::length(code)), index_(index),
          flag_(flag) {}

    std::string code_;
    size_t codeLength_;
    uint32_t index_;
    PhraseFlag flag_;
};
} // namespace libime

#endif // _FCITX_LIBIME_TABLE_TABLEDECODER_P_H_
