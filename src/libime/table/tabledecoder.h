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
#ifndef _FCITX_LIBIME_TABLE_TABLEDECODER_H_
#define _FCITX_LIBIME_TABLE_TABLEDECODER_H_

#include "libimetable_export.h"
#include <libime/core/decoder.h>
#include <libime/table/tablebaseddictionary.h>

namespace libime {

class TableLatticeNode : public LatticeNode {
public:
    TableLatticeNode(boost::string_view word, WordIndex idx,
                     SegmentGraphPath path, const State &state, float cost,
                     boost::string_view aux)
        : LatticeNode(word, idx, path, state, cost, aux) {}

    const std::string &code() const { return aux_; }

private:
    std::string encodedTable_;
};

class LIBIMETABLE_EXPORT TableDecoder : public Decoder {
public:
    TableDecoder(const TableBasedDictionary *dict,
                 const LanguageModelBase *model)
        : Decoder(dict, model) {}

protected:
    LatticeNode *createLatticeNodeImpl(const SegmentGraphBase &graph,
                                       const LanguageModelBase *model,
                                       boost::string_view word, WordIndex idx,
                                       SegmentGraphPath path,
                                       const State &state, float cost,
                                       boost::string_view aux,
                                       bool onlyPath) const override;
};
}

#endif // _FCITX_LIBIME_TABLE_TABLEDECODER_H_
