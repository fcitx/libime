/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_INPUTBUFFER_H_
#define _FCITX_LIBIME_CORE_INPUTBUFFER_H_

#include "libimecore_export.h"
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <cstddef>
#include <fcitx-utils/inputbuffer.h>
#include <string_view>

namespace libime {
class InputBufferPrivate;

class LIBIMECORE_EXPORT InputBuffer : public fcitx::InputBuffer {
public:
    class iterator
        : public boost::iterator_facade<iterator, std::string_view,
                                        boost::bidirectional_traversal_tag,
                                        std::string_view> {
    public:
        iterator() = default;
        iterator(const InputBuffer *buffer, size_t idx)
            : buffer_(buffer), idx_(idx) {}

        bool equal(iterator const &other) const {
            return buffer_ == other.buffer_ && idx_ == other.idx_;
        }

        void increment() { idx_++; }

        void decrement() { idx_--; }

        std::string_view dereference() const { return buffer_->at(idx_); }

    private:
        const InputBuffer *buffer_ = nullptr;
        size_t idx_ = 0;
    };

    using fcitx::InputBuffer::InputBuffer;

    using fcitx::InputBuffer::type;
    // add one overload for string_view
    bool type(std::string_view s) { return type(s.data(), s.length()); }
    std::string_view at(size_t i) const;

    std::string_view operator[](size_t i) const { return at(i); }

    iterator begin() { return {this, 0}; }

    iterator end() { return {this, size()}; }
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_INPUTBUFFER_H_
