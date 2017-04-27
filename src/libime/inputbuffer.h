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
                                        boost::bidirectional_traversal_tag,
                                        boost::string_view> {
    public:
        iterator() {}
        iterator(const InputBuffer *buffer, size_t idx)
            : buffer_(buffer), idx_(idx) {}

        bool equal(iterator const &other) const {
            return buffer_ == other.buffer_ && idx_ == other.idx_;
        }

        void increment() { idx_++; }

        void decrement() { idx_--; }

        boost::string_view dereference() const { return buffer_->at(idx_); }

    private:
        const InputBuffer *buffer_ = nullptr;
        size_t idx_ = 0;
    };

    InputBuffer(bool asciiOnly = false);
    virtual ~InputBuffer();

    virtual void type(boost::string_view s);
    virtual void erase(size_t from, size_t to);
    virtual void setCursor(size_t cursor);

    size_t maxSize() const;
    void setMaxSize(size_t s);

    const std::string &userInput() const;
    void type(uint32_t unicode);
    size_t cursor() const;
    size_t cursorByChar() const;
    size_t size() const;
    boost::string_view at(size_t i) const;
    size_t sizeAt(size_t i) const;
    inline void del() {
        auto c = cursor();
        if (c < size()) {
            erase(c, c + 1);
        }
    }
    inline void backspace() {
        auto c = cursor();
        if (c > 0) {
            erase(c - 1, c);
        }
    }
    virtual void clear() { erase(0, size()); }

    boost::string_view operator[](size_t i) const { return at(i); }

    iterator begin() { return iterator(this, 0); }

    iterator end() { return iterator(this, size()); }

private:
    std::unique_ptr<InputBufferPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputBuffer);
};
}

#endif // _FCITX_LIBIME_INPUTBUFFER_H_
