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
#include "segmentgraph.h"
#include <cstdint>
#include <fcitx-utils/macros.h>
#include <memory>
#include <vector>

namespace libime {

class DecoderPrivate;
class Dictionary;
class LanguageModel;

typedef std::function<bool(const SegmentGraph &,
                           const std::vector<const SegmentGraphNode *> &,
                           boost::string_view, float &)>
    UnknownHandler;

class LIBIME_EXPORT Decoder {
public:
    Decoder(Dictionary *dict, LanguageModel *model);
    virtual ~Decoder();

    void decode(const SegmentGraph &graph, int nbest, double max, double min);

    void decode(const SegmentGraph &graph, int nbest);

    void setUnknownHandler(UnknownHandler handler);

private:
    std::unique_ptr<DecoderPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Decoder);
};
}

#endif // _LIBIME_DECODER_H_
