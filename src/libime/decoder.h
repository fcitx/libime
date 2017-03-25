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
#include "segments.h"
#include <cstdint>
#include <fcitx-utils/macros.h>
#include <memory>
#include <vector>

namespace libime {

class DecoderPrivate;
class Dictionary;
class LanguageModel;

class LIBIME_EXPORT Decoder {
public:
    Decoder(Dictionary *dict, LanguageModel *model);
    virtual ~Decoder();

    void decode(const Segments &input, int nbest, const std::vector<int> &constrains);

    void decode(const Segments &input, int nbest, const std::vector<int> &constrains, double max,
                double min);

private:
    std::unique_ptr<DecoderPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Decoder);
};
}

#endif // _LIBIME_DECODER_H_
