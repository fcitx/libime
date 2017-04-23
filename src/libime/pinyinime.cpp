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
    PinyinIMEPrivate(std::unique_ptr<PinyinDictionary> dict,
                     std::unique_ptr<LanguageModelBase> model)
        : dict_(std::move(dict)), model_(std::move(model)),
          decoder_(std::make_unique<PinyinDecoder>(dict_.get(), model_.get())) {
    }

    PinyinFuzzyFlags flags_;
    std::unique_ptr<PinyinDictionary> dict_;
    std::unique_ptr<LanguageModelBase> model_;
    std::unique_ptr<PinyinDecoder> decoder_;
    size_t nbest_ = 1;
};

PinyinIME::PinyinIME(std::unique_ptr<PinyinDictionary> dict,
                     std::unique_ptr<LanguageModelBase> model)
    : d_ptr(std::make_unique<PinyinIMEPrivate>(std::move(dict),
                                               std::move(model))) {}

PinyinIME::~PinyinIME() {}

PinyinFuzzyFlags PinyinIME::fuzzyFlags() const {
    FCITX_D();
    return d->flags_;
}

void PinyinIME::setFuzzyFlags(PinyinFuzzyFlags flags) {
    FCITX_D();
    d->flags_ = flags;
    d->dict_->setFuzzyFlags(flags);
}

PinyinDictionary *PinyinIME::dict() {
    FCITX_D();
    return d->dict_.get();
}

const PinyinDictionary *PinyinIME::dict() const {
    FCITX_D();
    return d->dict_.get();
}

const PinyinDecoder *PinyinIME::decoder() const {
    FCITX_D();
    return d->decoder_.get();
}

const LanguageModelBase *PinyinIME::model() const {
    FCITX_D();
    return d->model_.get();
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
