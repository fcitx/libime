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
#include "segments.h"
#include <boost/iterator/function_input_iterator.hpp>
#include <exception>
#include <fcitx-utils/utf8.h>
#include <numeric>

namespace libime {

class InputBufferPrivate {
public:
    InputBufferPrivate(bool asciiOnly) : asciiOnly_(asciiOnly) {}
    const bool asciiOnly_;
    std::string input_;
    size_t cursor_ = 0;
    std::vector<size_t> idx_; // utf8 lengthindex helper
};

InputBuffer::InputBuffer(bool asciiOnly) : d_ptr(std::make_unique<InputBufferPrivate>(asciiOnly)) {}

InputBuffer::~InputBuffer() {}

void InputBuffer::type(uint32_t c) { type(fcitx::utf8::UCS4ToUTF8(c)); }

const std::string &InputBuffer::userInput() const {
    FCITX_D();
    return d->input_;
}

void InputBuffer::type(const std::string &s) {
    FCITX_D();
    auto len = fcitx::utf8::lengthValidated(s);
    if (len == fcitx::utf8::INVALID_LENGTH) {
        throw std::invalid_argument("Invalid UTF-8 string");
    }
    if (d->asciiOnly_ && len != s.size()) {
        throw std::invalid_argument("ascii only buffer only accept ascii only string");
    }
    d->input_.insert(cursorByChar(), s);
    if (!d->asciiOnly_) {
        auto iter = s.begin();
        std::function<size_t()> func = [&iter]() {
            auto next = fcitx::utf8::nextChar(iter);
            auto diff = std::distance(iter, next);
            iter = next;
            return diff;
        };
        d->idx_.insert(d->idx_.begin() + d->cursor_,
                       boost::make_function_input_iterator(func, static_cast<size_t>(0)),
                       boost::make_function_input_iterator(func, len));
    }
    d->cursor_ += len;
}

size_t InputBuffer::cursorByChar() const {
    FCITX_D();
    if (d->asciiOnly_) {
        return d->cursor_;
    }
    return std::accumulate(d->idx_.begin(), d->idx_.begin() + d->cursor_, 0);
}

size_t InputBuffer::cursor() const {
    FCITX_D();
    return d->cursor_;
}

size_t InputBuffer::size() const {
    FCITX_D();
    return d->asciiOnly_ ? d->input_.size() : d->idx_.size();
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
            fromByChar = std::accumulate(d->idx_.begin(), d->idx_.begin() + from, 0);
            lengthByChar = std::accumulate(d->idx_.begin() + from, d->idx_.begin() + to, 0);
            d->idx_.erase(std::next(d->idx_.begin(), from), std::next(d->idx_.begin(), to));
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
        auto from = std::accumulate(d->idx_.begin(), d->idx_.begin() + i, 0);
        return boost::string_view(d->input_).substr(from, d->idx_[i]);
    }
}

size_t InputBuffer::sizeAt(size_t i) const {
    FCITX_D();
    if (d->asciiOnly_) {
        return 1;
    } else {
        return d->idx_[i];
    }
}
}
