/*
 * Copyright (C) 2015~2017 by CSSlayer
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

#ifndef _FCITX_LIBIME_TABLE_TABLERULE_H_
#define _FCITX_LIBIME_TABLE_TABLERULE_H_

#include "libimetable_export.h"

#include <boost/algorithm/string.hpp>
#include <cstdint>
#include <iostream>
#include <libime/core/utils.h>
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
    TableRuleEntry(TableRuleEntryFlag _flag = TableRuleEntryFlag::FromFront,
                   uint8_t _character = 0, uint8_t _encodingIndex = 0)
        : flag_(_flag), character_(_character), encodingIndex_(_encodingIndex) {
    }

    explicit TableRuleEntry(std::istream &in) {
        throw_if_io_fail(unmarshall(in, flag_));
        throw_if_io_fail(unmarshall(in, character_));
        throw_if_io_fail(unmarshall(in, encodingIndex_));
    }

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(TableRuleEntry);

    friend std::ostream &operator<<(std::ostream &out,
                                    const TableRuleEntry &r) {
        marshall(out, r.flag_) && marshall(out, r.character_) &&
            marshall(out, r.encodingIndex_);
        return out;
    }

    bool isPlaceHolder() const {
        return character_ == 0 || encodingIndex_ == 0;
    }

    TableRuleEntryFlag flag() const { return flag_; }
    uint8_t character() const { return character_; }
    uint8_t encodingIndex() const { return encodingIndex_; }

private:
    TableRuleEntryFlag flag_;
    uint8_t character_;
    uint8_t encodingIndex_;
};

class TableRule {
public:
    TableRule(const std::string &ruleString, unsigned int maxLength) {
        if (!ruleString[0]) {
            throw std::invalid_argument("invalid rule string");
        }

        switch (ruleString[0]) {
        case 'e':
        case 'E':
            flag_ = TableRuleFlag::LengthEqual;
            break;

        case 'a':
        case 'A':
            flag_ = TableRuleFlag::LengthLongerThan;
            break;

        default:
            throw std::invalid_argument("invalid rule string");
        }

        auto equalSignPos = ruleString.find('=', 1);
        if (equalSignPos == std::string::npos) {
            throw std::invalid_argument("invalid rule string");
        }

        auto afterEqualSign =
            std::string_view(ruleString).substr(equalSignPos + 1);
        std::vector<std::string> entryStrings;
        boost::split(entryStrings, afterEqualSign, boost::is_any_of("+"));
        if (entryStrings.empty() || entryStrings.size() > maxLength) {
            throw std::invalid_argument("invalid rule string");
        }

        auto beforeEqualSign =
            std::string_view(ruleString).substr(0, equalSignPos);
        if (beforeEqualSign.size() != 2 || !isdigit(beforeEqualSign[1])) {
            throw std::invalid_argument("invalid rule string");
        }

        phraseLength_ = beforeEqualSign[1] - '0';
        if (phraseLength_ <= 0 || phraseLength_ > maxLength) {
            throw std::invalid_argument("Invalid phrase length");
        }

        unsigned int idx = 0;
        for (const auto &entryString : entryStrings) {
            idx++;
            TableRuleEntryFlag entryFlag;
            switch (entryString[0]) {
            case 'p':
            case 'P':
                entryFlag = TableRuleEntryFlag::FromFront;
                break;
            case 'n':
            case 'N':
                entryFlag = TableRuleEntryFlag::FromBack;
                break;
            default:
                throw std::invalid_argument("invalid rule entry flag");
            }

            if (entryString.size() != 3 || !isdigit(entryString[1]) ||
                !isdigit(entryString[2])) {
                throw std::invalid_argument("invalid rule entry");
            }

            int8_t character = entryString[1] - '0';     // 0 ~ maxLength
            int8_t encodingIndex = entryString[2] - '0'; // 0 ~ maxLength
            if (character < 0 || character > static_cast<int>(maxLength) ||
                encodingIndex < 0 ||
                encodingIndex > static_cast<int>(maxLength) ||
                ((character == 0) ^ (encodingIndex == 0))) {
                throw std::invalid_argument("invalid rule entry");
            }

            entries_.push_back(
                TableRuleEntry(entryFlag, character, encodingIndex));
        }
    }

    TableRule(TableRuleFlag _flag = TableRuleFlag::LengthEqual,
              int _phraseLength = 0, std::vector<TableRuleEntry> _entries = {})
        : flag_(_flag), phraseLength_(_phraseLength),
          entries_(std::move(_entries)) {}

    explicit TableRule(std::istream &in) {
        uint32_t size;
        throw_if_io_fail(unmarshall(in, flag_));
        throw_if_io_fail(unmarshall(in, phraseLength_));
        throw_if_io_fail(unmarshall(in, size));
        entries_.reserve(size);
        for (auto i = 0U; i < size; i++) {
            entries_.emplace_back(in);
        }
    }

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

    std::string name() const {
        std::string result;
        result += ((flag_ == TableRuleFlag::LengthEqual) ? 'e' : 'a');
        result += std::to_string(phraseLength_);

        return result;
    }

    std::string toString() const {
        std::string result;

        result += name();
        result += '=';
        bool first = true;
        for (const auto &entry : entries_) {
            if (first) {
                first = false;
            } else {
                result += '+';
            }
            result +=
                ((entry.flag() == TableRuleEntryFlag::FromFront) ? 'p' : 'n');
            result += static_cast<char>('0' + entry.character());
            result += static_cast<char>('0' + entry.encodingIndex());
        }
        return result;
    }

    TableRuleFlag flag() const { return flag_; }
    uint8_t phraseLength() const { return phraseLength_; }
    const std::vector<TableRuleEntry> &entries() const { return entries_; }
    size_t codeLength() const {
        size_t sum = 0;
        for (const auto &entry : entries_) {
            if (entry.isPlaceHolder()) {
                continue;
            }
            sum += 1;
        }
        return sum;
    }

private:
    TableRuleFlag flag_ = TableRuleFlag::LengthLongerThan;
    uint8_t phraseLength_ = 0;
    std::vector<TableRuleEntry> entries_;
};
} // namespace libime

#endif // _FCITX_LIBIME_TABLE_TABLERULE_H_
