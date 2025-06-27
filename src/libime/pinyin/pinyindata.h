/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_PINYIN_PINYINDATA_H_
#define _FCITX_LIBIME_PINYIN_PINYINDATA_H_

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <boost/container_hash/hash.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index_container.hpp>
#include <libime/pinyin/libimepinyin_export.h>
#include <libime/pinyin/pinyinencoder.h>

namespace libime {
struct LIBIMEPINYIN_EXPORT PinyinHash {
    std::size_t operator()(std::string_view const &val) const {
        return boost::hash_range(val.begin(), val.end());
    }
};

class LIBIMEPINYIN_EXPORT PinyinEntry {
public:
    PinyinEntry(const char *pinyin, PinyinInitial initial, PinyinFinal final,
                PinyinFuzzyFlags flags)
        : pinyin_(pinyin), initial_(initial), final_(final), flags_(flags) {}

    std::string_view pinyinView() const { return pinyin_; }
    constexpr const std::string &pinyin() const { return pinyin_; }
    constexpr PinyinInitial initial() const { return initial_; }
    constexpr PinyinFinal final() const { return final_; }
    constexpr PinyinFuzzyFlags flags() const { return flags_; }

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

LIBIMEPINYIN_EXPORT const PinyinMap &getPinyinMap();
LIBIMEPINYIN_EXPORT const PinyinMap &getPinyinMapV2();
LIBIMEPINYIN_EXPORT const std::vector<bool> &getEncodedInitialFinal();

LIBIMEPINYIN_EXPORT const
    std::unordered_map<std::string, std::pair<std::string, std::string>> &
    getInnerSegment();
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINDATA_H_
