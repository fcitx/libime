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

#include <string>
#include <vector>

namespace libime {

class Segments {
public:
    void clear() {
        data_.clear();
        idx_.clear();
    }

    void append(const std::string &s) { return append(s.c_str(), s.size()); }

    void append(const char *s, size_t n) {
        idx_.push_back(data_.size());
        data_.insert(data_.end(), s, s + n);
    }

    void pop() {
        auto i = idx_.back();
        idx_.pop_back();
        data_.erase(std::next(data_.begin(), i), data_.end());
    }

    size_t size() const { return idx_.size(); }

    std::vector<char> at(size_t i) const {
        auto start = idx_[i];
        auto end = (i + 1 < idx_.size()) ? idx_[i + 1] : data_.size();
        return {std::next(data_.begin(), start), std::next(data_.begin(), end)};
    }

    const char *right(size_t i) const { return data_.data() + idx_[i]; }

    size_t rightSize(int i) const { return data_.size() - idx_[i]; }

private:
    std::vector<char> data_;
    std::vector<int> idx_;
};
}

#endif // _LIBIME_SEGMENTS_H_
