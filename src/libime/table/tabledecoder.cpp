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

#include "tabledecoder.h"
#include <cmath>

namespace libime {

LatticeNode *TableDecoder::createLatticeNodeImpl(
    const SegmentGraphBase &graph, const LanguageModelBase *model,
    boost::string_view word, WordIndex idx, SegmentGraphPath path,
    const State &state, float cost, boost::string_view aux,
    bool onlyPath) const {
    if (model->isUnknown(idx, word)) {
        // we don't really care about a lot of unknown single character
        // which is not used for candidates
        if (aux.size() == 2 && path.front() != &graph.start() && !onlyPath) {
            return nullptr;
        }
    }

    return new TableLatticeNode(word, idx, std::move(path), std::move(state),
                                cost, aux);
}
}
