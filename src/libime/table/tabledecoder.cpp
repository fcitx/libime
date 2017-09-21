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

#include "tabledecoder_p.h"
#include <cmath>

namespace libime {

uint32_t TableLatticeNode::index() const {
    return d_ptr ? d_ptr->index_ : 0xFFFFFFFFu;
}

PhraseFlag TableLatticeNode::flag() const {
    return d_ptr ? d_ptr->flag_ : PhraseFlag::None;
}

const std::string &TableLatticeNode::code() const {
    static const std::string empty;
    if (!d_ptr) {
        return empty;
    }
    return d_ptr->code_;
}

TableLatticeNode::TableLatticeNode(
    boost::string_view word, WordIndex idx, SegmentGraphPath path,
    const State &state, float cost,
    std::unique_ptr<TableLatticeNodePrivate> data)
    : LatticeNode(word, idx, std::move(path), state, cost),
      d_ptr(std::move(data)) {}

TableLatticeNode::~TableLatticeNode() = default;

LatticeNode *TableDecoder::createLatticeNodeImpl(
    const SegmentGraphBase &graph, const LanguageModelBase *model,
    boost::string_view word, WordIndex idx, SegmentGraphPath path,
    const State &state, float cost, std::unique_ptr<LatticeNodeData> data,
    bool) const {
    std::unique_ptr<TableLatticeNodePrivate> tableData(
        static_cast<TableLatticeNodePrivate *>(data.release()));
    return new TableLatticeNode(word, idx, std::move(path), state, cost,
                                std::move(tableData));
}
}
