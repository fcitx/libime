/*
 * SPDX-FileCopyrightText: 2024-2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "pinyincorrectionprofile.h"
#include "pinyindata.h"
#include "pinyinencoder.h"

namespace libime {

namespace {

/*
 * Helper function to create mapping based on keyboard rows.
 * Function assume that the key can only be corrected to the key adjcent to it.
 */
std::unordered_map<char, std::vector<char>>
mappingFromRows(const std::vector<std::string> &rows) {
    std::unordered_map<char, std::vector<char>> result;
    for (const auto &row : rows) {
        for (size_t i = 0; i < row.size(); i++) {
            std::vector<char> items;
            if (i > 0) {
                items.push_back(row[i - 1]);
            }
            if (i + 1 < row.size()) {
                items.push_back(row[i + 1]);
            }
            result[row[i]] = std::move(items);
        }
    }
    return result;
}

std::unordered_map<char, std::vector<char>>
getProfileMapping(BuiltinPinyinCorrectionProfile profile) {
    switch (profile) {
    case BuiltinPinyinCorrectionProfile::Qwerty:
        return mappingFromRows({"qwertyuiop", "asdfghjkl", "zxcvbnm"});
    }

    return {};
}
} // namespace

class PinyinCorrectionProfilePrivate {
public:
    PinyinMap pinyinMap_;
};

PinyinCorrectionProfile::PinyinCorrectionProfile(
    BuiltinPinyinCorrectionProfile profile)
    : PinyinCorrectionProfile(getProfileMapping(profile)) {}

PinyinCorrectionProfile::PinyinCorrectionProfile(
    const std::unordered_map<char, std::vector<char>> &mapping)
    : d_ptr(std::make_unique<PinyinCorrectionProfilePrivate>()) {
    FCITX_D();
    // Fill with the original pinyin map.
    d->pinyinMap_ = getPinyinMapV2();
    if (mapping.empty()) {
        return;
    }
    // Re-map all entry with the correction mapping.
    std::vector<PinyinEntry> newEntries;
    for (const auto &item : d->pinyinMap_) {
        for (size_t i = 0; i < item.pinyin().size(); i++) {
            auto chr = item.pinyin()[i];
            auto swap = mapping.find(chr);
            if (swap == mapping.end() || swap->second.empty()) {
                continue;
            }
            auto newEntry = item.pinyin();
            for (auto sub : swap->second) {
                newEntry[i] = sub;
                newEntries.push_back(
                    PinyinEntry(newEntry.data(), item.initial(), item.final(),
                                item.flags() | PinyinFuzzyFlag::Correction));
                newEntry[i] = chr;
            }
        }
    }
    for (const auto &newEntry : newEntries) {
        d->pinyinMap_.insert(newEntry);
    }
}

PinyinCorrectionProfile::~PinyinCorrectionProfile() = default;

const PinyinMap &PinyinCorrectionProfile::pinyinMap() const {
    FCITX_D();
    return d->pinyinMap_;
}

} // namespace libime