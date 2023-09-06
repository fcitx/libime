/*
 * SPDX-FileCopyrightText: 2015-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef _FCITX_LIBIME_TABLE_TABLERULE_H_
#define _FCITX_LIBIME_TABLE_TABLERULE_H_

#include "libimetable_export.h"

#include <boost/algorithm/string.hpp>
#include <cstdint>
#include <fcitx-utils/charutils.h>
#include <iostream>
#include <libime/core/utils.h>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace libime {
enum class TableRuleEntryFlag : std::uint32_t { FromFront, FromBack };

enum class TableRuleFlag : std::uint32_t { LengthLongerThan, LengthEqual };

class LIBIMETABLE_EXPORT TableRuleEntry {
public:
    TableRuleEntry(TableRuleEntryFlag flag = TableRuleEntryFlag::FromFront,
                   uint8_t character = 0, uint8_t encodingIndex = 0);

    explicit TableRuleEntry(std::istream &in);

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(TableRuleEntry);

    friend std::ostream &operator<<(std::ostream &out,
                                    const TableRuleEntry &r) {
        marshall(out, r.flag_) && marshall(out, r.character_) &&
            marshall(out, r.encodingIndex_);
        return out;
    }

    bool isPlaceHolder() const;

    TableRuleEntryFlag flag() const { return flag_; }
    uint8_t character() const { return character_; }
    LIBIMETABLE_DEPRECATED uint8_t encodingIndex() const {
        return encodingIndex_;
    }
    int index() const;

private:
    TableRuleEntryFlag flag_;
    uint8_t character_;
    uint8_t encodingIndex_;
};

class LIBIMETABLE_EXPORT TableRule {
public:
    TableRule(const std::string &ruleString, unsigned int maxLength);

    TableRule(TableRuleFlag _flag = TableRuleFlag::LengthEqual,
              int _phraseLength = 0, std::vector<TableRuleEntry> _entries = {});

    explicit TableRule(std::istream &in);

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(TableRule)

    friend std::ostream &operator<<(std::ostream &out, const TableRule &r) {
        if (marshall(out, r.flag_) && marshall(out, r.phraseLength_) &&
            marshall(out, static_cast<uint32_t>(r.entries_.size()))) {
            for (const auto &entry : r.entries_) {
                if (!(out << entry)) {
                    break;
                }
            }
        }
        return out;
    }

    std::string name() const;

    std::string toString() const;

    TableRuleFlag flag() const { return flag_; }
    uint8_t phraseLength() const { return phraseLength_; }
    const std::vector<TableRuleEntry> &entries() const { return entries_; }
    size_t codeLength() const;

private:
    TableRuleFlag flag_ = TableRuleFlag::LengthLongerThan;
    uint8_t phraseLength_ = 0;
    std::vector<TableRuleEntry> entries_;
};
} // namespace libime

#endif // _FCITX_LIBIME_TABLE_TABLERULE_H_
