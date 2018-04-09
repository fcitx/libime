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
#include "inputbuffer.h"
#include <boost/iterator/function_input_iterator.hpp>
#include <boost/range/iterator_range.hpp>
#include <exception>
#include <fcitx-utils/utf8.h>
#include <numeric>
#include <vector>

namespace libime {

boost::string_view InputBuffer::at(size_t i) const {
    size_t start, end;
    std::tie(start, end) = rangeAt(i);
    return boost::string_view(userInput()).substr(start, end - start);
}
} // namespace libime
