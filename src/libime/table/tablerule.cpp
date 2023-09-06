/*
 * SPDX-FileCopyrightText: 2017-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "tablerule.h"

namespace libime {

namespace {

constexpr int TAIL_OFFSET = 0x80;

int8_t toIndex(uint8_t index) {
    if (index < TAIL_OFFSET) {
        return index;
    }
    return -static_cast<int8_t>(index - TAIL_OFFSET + 1);
}

uint8_t fromIndex(int8_t index) {
    if (index >= 0) {
        return index;
    }
    return (-index) + TAIL_OFFSET - 1;
}

} // namespace

TableRuleEntry::TableRuleEntry(TableRuleEntryFlag flag, uint8_t character,
                               uint8_t encodingIndex)
    : flag_(flag), character_(character), encodingIndex_(encodingIndex) {}

TableRuleEntry::TableRuleEntry(std::istream &in) {
    throw_if_io_fail(unmarshall(in, flag_));
    throw_if_io_fail(unmarshall(in, character_));
    throw_if_io_fail(unmarshall(in, encodingIndex_));
}

bool TableRuleEntry::isPlaceHolder() const {
    return character_ == 0 || index() == 0;
}

int TableRuleEntry::index() const { return toIndex(encodingIndex_); }

TableRule::TableRule(const std::string &ruleString, unsigned int maxLength) {
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

    auto afterEqualSign = std::string_view(ruleString).substr(equalSignPos + 1);
    std::vector<std::string> entryStrings;
    boost::split(entryStrings, afterEqualSign, boost::is_any_of("+"));
    if (entryStrings.empty() || entryStrings.size() > maxLength) {
        throw std::invalid_argument("invalid rule string");
    }

    auto beforeEqualSign = std::string_view(ruleString).substr(0, equalSignPos);
    if (beforeEqualSign.size() != 2 ||
        !fcitx::charutils::isdigit(beforeEqualSign[1])) {
        throw std::invalid_argument("invalid rule string");
    }

    phraseLength_ = beforeEqualSign[1] - '0';
    if (phraseLength_ <= 0 || phraseLength_ > maxLength) {
        throw std::invalid_argument("Invalid phrase length");
    }

    for (const auto &entryString : entryStrings) {
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

        if (entryString.size() != 3 ||
            !fcitx::charutils::isdigit(entryString[1]) ||
            !(fcitx::charutils::isdigit(entryString[2]) ||
              fcitx::charutils::isupper(entryString[2]) ||
              fcitx::charutils::islower(entryString[2]))) {
            throw std::invalid_argument("invalid rule entry");
        }

        int8_t character = entryString[1] - '0'; // 0 ~ maxLength
        int8_t index;
        if (fcitx::charutils::isdigit(entryString[2])) {
            index = entryString[2] - '0';
        } else {
            index = fcitx::charutils::tolower(entryString[2]) - 'z' - 1;
        }
        if (character < 0 || character > static_cast<int>(maxLength) ||
            std::abs(index) > static_cast<int>(maxLength) ||
            ((character == 0) ^ (index == 0))) {
            throw std::invalid_argument("invalid rule entry");
        }

        entries_.push_back(
            TableRuleEntry(entryFlag, character, fromIndex(index)));
    }
}
TableRule::TableRule(TableRuleFlag _flag, int _phraseLength,
                     std::vector<TableRuleEntry> _entries)
    : flag_(_flag), phraseLength_(_phraseLength),
      entries_(std::move(_entries)) {}
TableRule::TableRule(std::istream &in) {
    uint32_t size = 0;
    throw_if_io_fail(unmarshall(in, flag_));
    throw_if_io_fail(unmarshall(in, phraseLength_));
    throw_if_io_fail(unmarshall(in, size));
    entries_.reserve(size);
    for (auto i = 0U; i < size; i++) {
        entries_.emplace_back(in);
    }
}

std::string TableRule::name() const {
    std::string result;
    result += ((flag_ == TableRuleFlag::LengthEqual) ? 'e' : 'a');
    result += std::to_string(phraseLength_);

    return result;
}

std::string TableRule::toString() const {
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
        result += ((entry.flag() == TableRuleEntryFlag::FromFront) ? 'p' : 'n');
        result += static_cast<char>('0' + entry.character());
        auto index = entry.index();
        if (index >= 0) {
            result += '0' + index;
        } else {
            result += 'z' + index + 1;
        }
    }
    return result;
}

size_t TableRule::codeLength() const {
    size_t sum = 0;
    for (const auto &entry : entries_) {
        if (entry.isPlaceHolder()) {
            continue;
        }
        sum += 1;
    }
    return sum;
}
} // namespace libime