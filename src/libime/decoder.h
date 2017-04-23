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
#ifndef _LIBIME_DECODER_H_
#define _LIBIME_DECODER_H_

#include "dictionary.h"
#include "lattice.h"
#include "segmentgraph.h"
#include <cstdint>
#include <fcitx-utils/macros.h>
#include <memory>
#include <vector>

namespace libime {

class DecoderPrivate;
class Dictionary;
class LanguageModel;

class LIBIME_EXPORT Decoder {
    friend class DecoderPrivate;

public:
    Decoder(Dictionary *dict, LanguageModelBase *model);
    virtual ~Decoder();

    Lattice decode(const SegmentGraph &graph, size_t nbest = 1,
                   float max = std::numeric_limits<float>::max(),
                   float min = -std::numeric_limits<float>::max(),
                   State state = {}) const;

protected:
    inline LatticeNode *createLatticeNode(LanguageModelBase *model,
                                          boost::string_view word,
                                          WordIndex idx, SegmentGraphPath path,
                                          float cost = 0, State state = {},
                                          boost::string_view aux = "") const {
        return createLatticeNodeImpl(model, word, idx, std::move(path), cost,
                                     std::move(state), aux);
    }
    virtual LatticeNode *
    createLatticeNodeImpl(LanguageModelBase *model, boost::string_view word,
                          WordIndex idx, SegmentGraphPath path, float cost,
                          State state, boost::string_view aux) const;

private:
    std::unique_ptr<DecoderPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Decoder);
};
}

#endif // _LIBIME_DECODER_H_
