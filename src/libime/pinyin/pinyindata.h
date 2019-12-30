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
#ifndef _FCITX_LIBIME_PINYIN_PINYINDATA_H_
#define _FCITX_LIBIME_PINYIN_PINYINDATA_H_

#include "libimepinyin_export.h"
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index_container.hpp>
#include <libime/pinyin/pinyinencoder.h>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace libime {
struct PinyinHash : std::unary_function<std::string_view, std::size_t> {
    std::size_t operator()(std::string_view const &val) const {
        return boost::hash_range(val.begin(), val.end());
    }
};

class PinyinEntry {
public:
    PinyinEntry(const char *pinyin, PinyinInitial initial, PinyinFinal final,
                PinyinFuzzyFlags flags)
        : pinyin_(pinyin), initial_(initial), final_(final), flags_(flags) {}

    std::string_view pinyinView() const { return pinyin_; }
    const std::string &pinyin() const { return pinyin_; }
    PinyinInitial initial() const { return initial_; }
    PinyinFinal final() const { return final_; }
    PinyinFuzzyFlags flags() const { return flags_; }

private:
    std::string pinyin_;
    PinyinInitial initial_;
    PinyinFinal final_;
    PinyinFuzzyFlags flags_;
};

using PinyinMap = boost::multi_index_container<
    PinyinEntry,
    boost::multi_index::indexed_by<boost::multi_index::hashed_non_unique<
        boost::multi_index::const_mem_fun<PinyinEntry, std::string_view,
                                          &PinyinEntry::pinyinView>,
        PinyinHash>>>;

LIBIMEPINYIN_EXPORT
const PinyinMap &getPinyinMap();
LIBIMEPINYIN_EXPORT const std::vector<bool> &getEncodedInitialFinal();

LIBIMEPINYIN_EXPORT const
    std::unordered_map<std::string, std::pair<std::string, std::string>> &
    getInnerSegment();
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINDATA_H_
