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
#include "pinyincontext.h"
#include "pinyinencoder.h"
#include "pinyinime.h"

namespace libime {

class PinyinContextPrivate {
public:
    PinyinContextPrivate(PinyinIME *ime) : ime_(ime) {}

    PinyinIME *ime_;
    SegmentGraph segs_;
};

PinyinContext::PinyinContext(PinyinIME *ime)
    : InputBuffer(true), d_ptr(std::make_unique<PinyinContextPrivate>(ime)) {}

PinyinContext::~PinyinContext() {}

void PinyinContext::type(boost::string_view s) {
    FCITX_D();
    auto c = cursor();
    InputBuffer::type(s);
    d->segs_ =
        PinyinEncoder::parseUserPinyin(userInput(), d->ime_->fuzzyFlags());
}

void PinyinContext::erase(size_t from, size_t to) {
    FCITX_D();
    InputBuffer::erase(from, to);
    d->segs_ =
        PinyinEncoder::parseUserPinyin(userInput(), d->ime_->fuzzyFlags());
}
}
