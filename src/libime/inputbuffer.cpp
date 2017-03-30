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
#include "inputbuffer.h"
#include "segmentpath.h"
#include <boost/iterator/function_input_iterator.hpp>
#include <boost/range/iterator_range.hpp>
#include <exception>
#include <fcitx-utils/utf8.h>
#include <numeric>

namespace libime {

class InputBufferPrivate {
public:
    InputBufferPrivate(bool asciiOnly) : asciiOnly_(asciiOnly) {}

    // make sure acc_[i] is valid, i \in [0, size()]
    // acc_[i] = sum(j \in 0..i-1 | sz_[j])
    void ensureAccTill(size_t i) const {
        if (accDirty_ <= i) {
            if (accDirty_ == 0) {
                // acc_[0] is always 0
                accDirty_++;
            }
            for (auto s : boost::make_iterator_range(
                     std::next(sz_.begin(), accDirty_ - 1),
                     std::next(sz_.begin(), i))) {
                acc_[accDirty_] = acc_[accDirty_ - 1] + s;
                accDirty_++;
            }
        }
    }

    const bool asciiOnly_;
    std::string input_;
    size_t cursor_ = 0;
    std::vector<size_t> sz_; // utf8 lengthindex helper
    mutable std::vector<size_t> acc_ = {0};
    mutable size_t accDirty_ = 0;
};

InputBuffer::InputBuffer(bool asciiOnly)
    : d_ptr(std::make_unique<InputBufferPrivate>(asciiOnly)) {}

InputBuffer::~InputBuffer() {}

void InputBuffer::type(uint32_t c) { type(fcitx::utf8::UCS4ToUTF8(c)); }

const std::string &InputBuffer::userInput() const {
    FCITX_D();
    return d->input_;
}

void InputBuffer::type(boost::string_view s) {
    FCITX_D();
    auto len = fcitx::utf8::lengthValidated(s.begin(), s.end());
    if (len == fcitx::utf8::INVALID_LENGTH) {
        throw std::invalid_argument("Invalid UTF-8 string");
    }
    if (d->asciiOnly_ && len != s.size()) {
        throw std::invalid_argument(
            "ascii only buffer only accept ascii only string");
    }
    d->input_.insert(std::next(d->input_.begin(), cursorByChar()), s.begin(),
                     s.end());
    if (!d->asciiOnly_) {
        auto iter = s.begin();
        std::function<size_t()> func = [&iter]() {
            auto next = fcitx::utf8::nextChar(iter);
            auto diff = std::distance(iter, next);
            iter = next;
            return diff;
        };
        d->sz_.insert(
            std::next(d->sz_.begin(), d->cursor_),
            boost::make_function_input_iterator(func, static_cast<size_t>(0)),
            boost::make_function_input_iterator(func, len));
        d->acc_.resize(d->sz_.size() + 1);
        d->accDirty_ = d->cursor_ > 0 ? d->cursor_ - 1 : 0;
    }
    d->cursor_ += len;
}

size_t InputBuffer::cursorByChar() const {
    FCITX_D();
    if (d->asciiOnly_) {
        return d->cursor_;
    }
    if (d->cursor_ == size()) {
        return d->input_.size();
    }
    d->ensureAccTill(d->cursor_);
    return d->acc_[d->cursor_];
}

size_t InputBuffer::cursor() const {
    FCITX_D();
    return d->cursor_;
}

size_t InputBuffer::size() const {
    FCITX_D();
    return d->asciiOnly_ ? d->input_.size() : d->sz_.size();
}

void InputBuffer::setCursor(size_t cursor) {
    FCITX_D();
    if (d->cursor_ > size()) {
        throw std::out_of_range("cursor position out of range");
    }
    d->cursor_ = cursor;
}

void InputBuffer::erase(size_t from, size_t to) {
    FCITX_D();
    if (from < to && to <= size()) {
        size_t fromByChar, lengthByChar;
        if (d->asciiOnly_) {
            fromByChar = from;
            lengthByChar = to - from;
        } else {
            d->ensureAccTill(to);
            fromByChar = d->acc_[from];
            lengthByChar = d->acc_[to] - fromByChar;
            d->sz_.erase(std::next(d->sz_.begin(), from),
                         std::next(d->sz_.begin(), to));
            d->accDirty_ = from;
            d->acc_.resize(d->sz_.size() + 1);
        }
        if (d->cursor_ > from) {
            if (d->cursor_ <= to) {
                d->cursor_ = from;
            } else {
                d->cursor_ -= to - from;
            }
        }
        d->input_.erase(fromByChar, lengthByChar);
    }
}

boost::string_view InputBuffer::at(size_t i) const {
    FCITX_D();
    if (i >= size()) {
        throw std::out_of_range("out of range");
    }
    if (d->asciiOnly_) {
        return boost::string_view(d->input_).substr(i, 1);
    } else {
        d->ensureAccTill(i);
        return boost::string_view(d->input_).substr(d->acc_[i], d->sz_[i]);
    }
}

size_t InputBuffer::sizeAt(size_t i) const {
    FCITX_D();
    if (d->asciiOnly_) {
        return 1;
    } else {
        return d->sz_[i];
    }
}
}
