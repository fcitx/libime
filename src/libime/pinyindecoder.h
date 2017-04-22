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
#ifndef _FCITX_LIBIME_PINYINDECODER_H_
#define _FCITX_LIBIME_PINYINDECODER_H_

#include "decoder.h"
#include "libime_export.h"
#include "pinyindictionary.h"

namespace libime {

class LIBIME_EXPORT PinyinDecoder : public Decoder {
public:
    PinyinDecoder(PinyinDictionary *dict, LanguageModel *model)
        : Decoder(dict, model) {}

protected:
    LatticeNode *
    createLatticeNodeImpl(LanguageModel *model, boost::string_view word,
                          WordIndex idx,
                          std::vector<const SegmentGraphNode *> path,
                          float cost, State state) const override;
};
}

#endif // _FCITX_LIBIME_PINYINDECODER_H_
