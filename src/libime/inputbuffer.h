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
#ifndef _FCITX_LIBIME_INPUTBUFFER_H_
#define _FCITX_LIBIME_INPUTBUFFER_H_

#include "libime_export.h"
#include <boost/iterator/iterator_facade.hpp>
#include <boost/utility/string_view.hpp>
#include <fcitx-utils/macros.h>
#include <memory>

namespace libime {
class InputBufferPrivate;

class LIBIME_EXPORT InputBuffer {
public:
    class iterator
        : public boost::iterator_facade<iterator, boost::string_view,
                                        boost::bidirectional_traversal_tag, boost::string_view> {
    public:
        iterator() {}
        iterator(const InputBuffer *buffer, size_t idx, size_t length)
            : buffer_(buffer), idx_(idx), length_(length) {}

        bool equal(iterator const &other) const {
            return buffer_ == other.buffer_ && idx_ == other.idx_ && length_ == other.length_;
        }

        void increment() {
            length_ += buffer_->sizeAt(idx_);
            idx_++;
        }

        void decrement() {
            idx_--;
            length_ -= buffer_->sizeAt(idx_);
        }

        boost::string_view dereference() const {
            return boost::string_view(buffer_->userInput()).substr(length_, buffer_->sizeAt(idx_));
        }

    private:
        const InputBuffer *buffer_ = nullptr;
        size_t idx_ = 0, length_ = 0;
    };

    InputBuffer(bool asciiOnly = false);
    virtual ~InputBuffer();

    const std::string &userInput() const;
    void type(uint32_t unicode);
    virtual void type(const std::string &s);
    size_t cursor() const;
    size_t cursorByChar() const;
    size_t size() const;
    void setCursor(size_t cursor);
    virtual void erase(size_t from, size_t to);
    boost::string_view at(size_t i) const;
    size_t sizeAt(size_t i) const;

    boost::string_view operator[](size_t i) const { return at(i); }

    iterator begin() { return iterator(this, 0, 0); }

    iterator end() { return iterator(this, size(), userInput().size()); }

private:
    std::unique_ptr<InputBufferPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputBuffer);
};
}

#endif // _FCITX_LIBIME_INPUTBUFFER_H_
