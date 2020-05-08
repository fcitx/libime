/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "inputbuffer.h"
#include <boost/iterator/function_input_iterator.hpp>
#include <boost/range/iterator_range.hpp>
#include <exception>
#include <fcitx-utils/utf8.h>
#include <numeric>
#include <vector>

namespace libime {

std::string_view InputBuffer::at(size_t i) const {
    size_t start, end;
    std::tie(start, end) = rangeAt(i);
    return std::string_view(userInput()).substr(start, end - start);
}
} // namespace libime
