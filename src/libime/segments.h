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
#ifndef _LIBIME_SEGMENTS_H_
#define _LIBIME_SEGMENTS_H_

#include <boost/utility/string_view.hpp>
#include <vector>

namespace libime {

class Segments {
public:
    Segments(boost::string_view view, std::vector<size_t> idx)
        : data_(view), idx_(std::move(idx)) {}

    size_t size() const { return idx_.size(); }
    boost::string_view data() const { return data_; }

    boost::string_view at(size_t i) const {
        auto start = i == 0 ? 0 : idx_[i - 1];
        auto end = idx_[i];
        return data_.substr(start, end - start);
    }

    boost::string_view prefix(size_t i) const {
        auto end = idx_[i];
        return data_.substr(0, end);
    }

private:
    boost::string_view data_;
    std::vector<size_t> idx_;
};
}

#endif // _LIBIME_SEGMENTS_H_
