/*
 * Copyright (C) 2015~2017 by CSSlayer
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

#ifndef LIBIME_TABLERULE_H
#define LIBIME_TABLERULE_H

#include "libime_export.h"

#include "utils.h"
#include <boost/algorithm/string.hpp>
#include <boost/utility/string_view.hpp>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace libime {
enum class TableRuleEntryFlag : std::uint32_t { FromFront, FromBack };

enum class TableRuleFlag : std::uint32_t { LengthLongerThan, LengthEqual };

struct LIBIME_EXPORT TableRuleEntry {

    TableRuleEntry(TableRuleEntryFlag _flag = TableRuleEntryFlag::FromFront,
                   uint8_t _character = 0, uint8_t _encodingIndex = 0)
        : flag(_flag), character(_character), encodingIndex(_encodingIndex) {}

    TableRuleEntry(std::istream &in) {
        throw_if_io_fail(unmarshall(in, flag));
        throw_if_io_fail(unmarshall(in, character));
        throw_if_io_fail(unmarshall(in, encodingIndex));
    }

    friend std::ostream &operator<<(std::ostream &out,
                                    const TableRuleEntry &r) {
        marshall(out, r.flag) && marshall(out, r.character) &&
            marshall(out, r.encodingIndex);
        return out;
    }

    TableRuleEntryFlag flag;
    uint8_t character;
    uint8_t encodingIndex;
};

struct TableRule {

    TableRule(const std::string &ruleString, unsigned int maxLength) {
        if (!ruleString[0]) {
            throw std::invalid_argument("invalid rule string");
        }

        switch (ruleString[0]) {
        case 'e':
        case 'E':
            flag = TableRuleFlag::LengthEqual;
            break;

        case 'a':
        case 'A':
            flag = TableRuleFlag::LengthLongerThan;
            break;

        default:
            throw std::invalid_argument("invalid rule string");
        }

        auto equalSignPos = ruleString.find('=', 1);
        if (equalSignPos == std::string::npos) {
            throw std::invalid_argument("invalid rule string");
        }

        auto afterEqualSign =
            boost::string_view(ruleString).substr(equalSignPos + 1);
        std::vector<std::string> entryStrings;
        boost::split(entryStrings, afterEqualSign, boost::is_any_of("+"));
        if (entryStrings.empty() || entryStrings.size() > maxLength) {
            throw std::invalid_argument("invalid rule string");
        }

        auto beforeEqualSign =
            boost::string_view(ruleString).substr(0, equalSignPos);
        if (beforeEqualSign.size() != 2 || !isdigit(beforeEqualSign[1])) {
            throw std::invalid_argument("invalid rule string");
        }

        phraseLength = beforeEqualSign[1] - '0';
        if (phraseLength <= 0 || phraseLength > maxLength) {
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

            uint8_t character = entryString[1] - '0';     // 0 ~ maxLength
            uint8_t encodingIndex = entryString[2] - '0'; // 0 ~ maxLength
            if (character <= 0 || encodingIndex > maxLength ||
                encodingIndex <= 0 ||
                ((character == 0) ^ (encodingIndex == 0))) {
                throw std::invalid_argument("invalid rule entry");
            }

            entries.push_back(
                TableRuleEntry(entryFlag, character, encodingIndex));
        }
    }

    TableRule(TableRuleFlag _flag = TableRuleFlag::LengthEqual,
              int _phraseLength = 0,
              std::vector<TableRuleEntry> _entries = decltype(entries)())
        : flag(_flag), phraseLength(_phraseLength),
          entries(std::move(_entries)) {}

    TableRule(const TableRule &other)
        : TableRule(other.flag, other.phraseLength, other.entries) {}

    TableRule(TableRule &&other) noexcept
        : flag(other.flag), phraseLength(other.phraseLength),
          entries(std::move(other.entries)) {}

    TableRule(std::istream &in) {
        uint32_t size;
        throw_if_io_fail(unmarshall(in, flag));
        throw_if_io_fail(unmarshall(in, phraseLength));
        throw_if_io_fail(unmarshall(in, size));
        entries.reserve(size);
        for (auto i = 0U; i < size; i++) {
            entries.emplace_back(in);
        }
    }

    TableRule &operator=(TableRule r) noexcept {
        swap(*this, r);
        return *this;
    }

    friend std::ostream &operator<<(std::ostream &out, const TableRule &r) {
        if (marshall(out, r.flag) && marshall(out, r.phraseLength) &&
            marshall(out, static_cast<uint32_t>(r.entries.size()))) {
            for (const auto &entry : r.entries) {
                if (!(out << entry)) {
                    break;
                }
            }
        }
        return out;
    }

    friend void swap(TableRule &first, TableRule &second) noexcept {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two classes,
        // the two classes are effectively swapped
        swap(first.flag, second.flag);
        swap(first.phraseLength, second.phraseLength);
        swap(first.entries, second.entries);
    }

    std::string toString() const {
        std::stringstream ss;

        ss << ((flag == TableRuleFlag::LengthEqual) ? 'e' : 'a')
           << static_cast<char>('0' + phraseLength) << '=';
        bool first = true;
        for (const auto &entry : entries) {
            if (first) {
                first = false;
            } else {
                ss << '+';
            }
            ss << ((entry.flag == TableRuleEntryFlag::FromFront) ? 'p' : 'n')
               << static_cast<char>('0' + entry.character)
               << static_cast<char>('0' + entry.encodingIndex);
        }
        return ss.str();
    }

    TableRuleFlag flag;
    unsigned short phraseLength;
    std::vector<TableRuleEntry> entries;
};
}

#endif // LIBIME_TABLERULE_H
