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
#include "pinyinime.h"
#include "pinyindecoder.h"

namespace libime {

class PinyinIMEPrivate {
public:
    PinyinIMEPrivate(PinyinDictionary *dict, LanguageModel *model)
        : decoder_(std::make_unique<PinyinDecoder>(dict, model)),
          model_(model) {}

    PinyinFuzzyFlags flags_;
    std::unique_ptr<PinyinDecoder> decoder_;
    LanguageModel *model_;
    size_t nbest_ = 1;
};

PinyinIME::PinyinIME(PinyinDictionary *dict, LanguageModel *model)
    : d_ptr(std::make_unique<PinyinIMEPrivate>(dict, model)) {}

PinyinIME::~PinyinIME() {}

PinyinFuzzyFlags PinyinIME::fuzzyFlags() const {
    FCITX_D();
    return d->flags_;
}

void PinyinIME::setFuzzyFlags(PinyinFuzzyFlags flags) {
    FCITX_D();
    d->flags_ = flags;
}

PinyinDecoder *PinyinIME::decoder() const {
    FCITX_D();
    return d->decoder_.get();
}

LanguageModel *PinyinIME::model() const {
    FCITX_D();
    return d->model_;
}

size_t PinyinIME::nbest() const {
    FCITX_D();
    return d->nbest_;
}

void PinyinIME::setNBest(size_t n) {
    FCITX_D();
    d->nbest_ = n;
}
}
