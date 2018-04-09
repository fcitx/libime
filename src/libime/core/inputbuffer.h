/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _FCITX_LIBIME_CORE_INPUTBUFFER_H_
#define _FCITX_LIBIME_CORE_INPUTBUFFER_H_

#include "libimecore_export.h"
#include <boost/iterator/iterator_facade.hpp>
#include <boost/utility/string_view.hpp>
#include <fcitx-utils/inputbuffer.h>
#include <fcitx-utils/macros.h>
#include <memory>

namespace libime {
class InputBufferPrivate;

class LIBIMECORE_EXPORT InputBuffer : public fcitx::InputBuffer {
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

    using fcitx::InputBuffer::InputBuffer;

    using fcitx::InputBuffer::type;
    // add one overload for string_view
    void type(boost::string_view s) { return type(s.data(), s.length()); }
    boost::string_view at(size_t i) const;

    boost::string_view operator[](size_t i) const { return at(i); }

    iterator begin() { return iterator(this, 0); }

    iterator end() { return iterator(this, size()); }
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_INPUTBUFFER_H_
