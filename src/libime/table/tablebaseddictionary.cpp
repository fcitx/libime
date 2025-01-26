/*
 * SPDX-FileCopyrightText: 2015-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/table/tablebaseddictionary.h"
#include "autophrasedict.h"
#include "constants.h"
#include "libime/core/datrie.h"
#include "libime/core/dictionary.h"
#include "libime/core/languagemodel.h"
#include "libime/core/lattice.h"
#include "libime/core/segmentgraph.h"
#include "libime/core/utils.h"
#include "libime/core/zstdfilter.h"
#include "log.h"
#include "tablebaseddictionary_p.h"
#include "tabledecoder_p.h"
#include "tableoptions.h"
#include "tablerule.h"
#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/utf8.h>
#include <fstream>
#include <ios>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace libime {

namespace {

constexpr char keyValueSeparator = '\x01';
constexpr char keyValueSeparatorString[] = {keyValueSeparator, '\0'};
// "fc" t"ab"l"e"
constexpr uint32_t tableBinaryFormatMagic = 0x000fcabe;
constexpr uint32_t tableBinaryFormatVersion = 0x2;
constexpr uint32_t userTableBinaryFormatMagic = 0x356fcabe;
constexpr uint32_t userTableBinaryFormatVersion = 0x3;
constexpr uint32_t extraTableBinaryFormatMagic = 0x6b0fcabe;
constexpr uint32_t extraTableBinaryFormatVersion = 0x1;

enum {
    STR_KEYCODE,
    STR_CODELEN,
    STR_IGNORECHAR,
    STR_PINYIN,
    STR_PINYINLEN,
    STR_DATA,
    STR_RULE,
    STR_PROMPT,
    STR_CONSTRUCTPHRASE,
    STR_PHRASE,
    STR_LAST
};

enum class BuildPhase { PhaseConfig, PhaseRule, PhaseData, PhasePhrase };

const char *strConst[2][STR_LAST] = {
    {"键码=", "码长=", "规避字符=", "拼音=", "拼音长度=", "[数据]",
     "[组词规则]", "提示=", "构词=", "[词组]"},
    {"KeyCode=", "Length=", "InvalidChar=", "Pinyin=", "PinyinLength=",
     "[Data]", "[Rule]", "Prompt=", "ConstructPhrase=", "[Phrase]"}};

constexpr std::string_view UserDictAutoMark = "[Auto]";
constexpr std::string_view UserDictDeleteMark = "[Delete]";

// A better version of key + keyValueSeparator + value. It tries to avoid
// multiple allocation.
inline std::string generateTableEntry(std::string_view key,
                                      std::string_view value) {
    return fcitx::stringutils::concat(key, keyValueSeparatorString, value);
}

inline std::string generateTableEntry(uint32_t pinyinKey, std::string_view key,
                                      std::string_view value) {
    return fcitx::stringutils::concat(fcitx::utf8::UCS4ToUTF8(pinyinKey), key,
                                      keyValueSeparatorString, value);
}

void maybeUnescapeValue(std::string &value) {
    if (value.size() >= 2 && fcitx::stringutils::startsWith(value, '"') &&
        fcitx::stringutils::endsWith(value, '"')) {
        if (auto unescape = fcitx::stringutils::unescapeForValue(value)) {
            value = unescape.value();
        }
    }
}

std::string maybeEscapeValue(std::string_view value) {
    auto escaped = fcitx::stringutils::escapeForValue(value);
    if (escaped.size() != value.size()) {
        if (fcitx::stringutils::startsWith(escaped, "\"") &&
            fcitx::stringutils::endsWith(escaped, "\"")) {
            return escaped;
        }
        return fcitx::stringutils::concat("\"", escaped, "\"");
    }
    return std::string{value};
}

void updateReverseLookupEntry(DATrie<int32_t> &trie, std::string_view key,
                              std::string_view value,
                              DATrie<int32_t> *reverseTrie) {

    auto reverseEntry = generateTableEntry(value, "");
    bool insert = true;
    trie.foreach(reverseEntry,
                 [&trie, &key, &value, &insert, reverseTrie](
                     int32_t, size_t len, DATrie<int32_t>::position_type pos) {
                     if (key.length() > len) {
                         std::string oldKey;
                         trie.suffix(oldKey, len, pos);
                         trie.erase(pos);
                         if (reverseTrie) {
                             auto entry = generateTableEntry(oldKey, value);
                             reverseTrie->erase(entry);
                         }
                     } else {
                         insert = false;
                     }
                     return false;
                 });
    if (insert) {
        reverseEntry.append(key.begin(), key.end());
        trie.set(reverseEntry, 1);
        if (reverseTrie) {
            auto entry = generateTableEntry(key, value);
            reverseTrie->set(entry, 1);
        }
    }
}

void saveTrieToText(const DATrie<uint32_t> &trie, std::ostream &out) {
    std::string buf;
    std::vector<std::tuple<std::string, std::string, uint32_t>> temp;
    trie.foreach([&trie, &buf, &temp](uint32_t value, size_t _len,
                                      DATrie<int32_t>::position_type pos) {
        trie.suffix(buf, _len, pos);
        auto sep = buf.find(keyValueSeparator);
        std::string_view ref(buf);
        temp.emplace_back(ref.substr(0, sep), ref.substr(sep + 1), value);
        return true;
    });
    std::sort(temp.begin(), temp.end(), [](const auto &lhs, const auto &rhs) {
        return std::get<uint32_t>(lhs) < std::get<uint32_t>(rhs);
    });
    for (auto &item : temp) {
        out << std::get<0>(item) << " " << maybeEscapeValue(std::get<1>(item))
            << '\n';
    }
}

uint32_t maxValue(const DATrie<uint32_t> &trie) {
    uint32_t max = 0;
    trie.foreach(
        [&max](uint32_t value, size_t, DATrie<uint32_t>::position_type) {
            max = std::max(value + 1, max);
            return true;
        });
    return max;
}

bool insertOrUpdateTrie(DATrie<uint32_t> &trie, uint32_t &index,
                        std::string_view entry, bool updateExisting) {
    // Always insert to user even it is dup because we need to update the index.
    if (trie.hasExactMatch(entry) && !updateExisting) {
        return false;
    }
    trie.set(entry, index);
    index += 1;
    return true;
}

} // namespace

bool TableBasedDictionaryPrivate::validateKeyValue(std::string_view key,
                                                   std::string_view value,
                                                   PhraseFlag flag) const {
    FCITX_Q();
    auto keyLength = fcitx::utf8::lengthValidated(key);
    auto valueLength = fcitx::utf8::lengthValidated(value);
    if (keyLength == fcitx::utf8::INVALID_LENGTH ||
        valueLength == fcitx::utf8::INVALID_LENGTH ||
        (codeLength_ && flag != PhraseFlag::Pinyin &&
         !q->isValidLength(keyLength))) {
        return false;
    }
    if (!inputCode_.empty() && flag != PhraseFlag::Pinyin &&
        !q->isAllInputCode(key)) {
        return false;
    }

    return true;
}

std::pair<DATrie<uint32_t> *, uint32_t *>
TableBasedDictionaryPrivate::trieByFlag(PhraseFlag flag) {
    switch (flag) {
    case PhraseFlag::None:
    case PhraseFlag::Pinyin:
        return {&phraseTrie_, &phraseTrieIndex_};
        break;
    case PhraseFlag::User:
        return {&userTrie_, &userTrieIndex_};
        break;
    default:
        return {nullptr, nullptr};
    }
}

std::pair<const DATrie<uint32_t> *, const uint32_t *>
TableBasedDictionaryPrivate::trieByFlag(PhraseFlag flag) const {
    switch (flag) {
    case PhraseFlag::None:
    case PhraseFlag::Pinyin:
        return {&phraseTrie_, &phraseTrieIndex_};
        break;
    case PhraseFlag::User:
        return {&userTrie_, &userTrieIndex_};
        break;
    default:
        return {nullptr, nullptr};
    }
}

bool TableBasedDictionaryPrivate::insert(std::string_view key,
                                         std::string_view value,
                                         PhraseFlag flag) {
    DATrie<uint32_t> *trie;
    uint32_t *index;

    auto pair = trieByFlag(flag);
    trie = pair.first;
    index = pair.second;

    std::string entry;
    if (flag == PhraseFlag::Pinyin) {
        entry = generateTableEntry(pinyinKey_, key, value);
    } else {
        entry = generateTableEntry(key, value);
    }

    if (flag == PhraseFlag::User) {
        deletionTrie_.erase(entry);
    }

    return insertOrUpdateTrie(*trie, *index, entry, flag == PhraseFlag::User);
}

bool TableBasedDictionaryPrivate::matchTrie(
    const DATrie<uint32_t> &trie, uint32_t indexOffset, std::string_view code,
    TableMatchMode mode, PhraseFlag flag,
    const TableMatchCallback &callback) const {
    auto range = fcitx::utf8::MakeUTF8CharRange(code);
    std::vector<DATrie<uint32_t>::position_type> positions;
    positions.push_back(0);
    // BFS on trie.
    for (auto iter = std::begin(range), end = std::end(range); iter != end;
         iter++) {
        decltype(positions) newPositions;

        for (auto position : positions) {
            if (flag != PhraseFlag::Pinyin && *iter == options_.matchingKey() &&
                options_.matchingKey()) {
                for (auto code : inputCode_) {
                    auto curPos = position;
                    auto strCode = fcitx::utf8::UCS4ToUTF8(code);
                    auto result = trie.traverse(strCode, curPos);
                    if (!DATrie<unsigned int>::isNoPath(result)) {
                        newPositions.push_back(curPos);
                    }
                }
            } else {
                auto charRange = iter.charRange();
                std::string_view chr(
                    &*charRange.first,
                    std::distance(charRange.first, charRange.second));
                auto curPos = position;
                auto result = trie.traverse(chr, curPos);
                if (!DATrie<unsigned int>::isNoPath(result)) {
                    newPositions.push_back(curPos);
                }
            }
        }

        positions = std::move(newPositions);
    }

    auto matchWord = [&trie, &code, &callback, flag, mode,
                      indexOffset](uint32_t value, size_t len,
                                   DATrie<int32_t>::position_type pos) {
        std::string entry;
        trie.suffix(entry, code.size() + len, pos);
        auto sep = entry.find(keyValueSeparator, code.size());
        if (sep == std::string::npos) {
            return true;
        }

        auto view = std::string_view(entry);
        auto matchedCode = view.substr(0, sep);
        if (mode == TableMatchMode::Prefix ||
            (mode == TableMatchMode::Exact &&
             fcitx::utf8::length(matchedCode) == fcitx::utf8::length(code))) {
            // Remove pinyinKey.
            if (flag == PhraseFlag::Pinyin) {
                matchedCode.remove_prefix(
                    fcitx::utf8::ncharByteLength(matchedCode.begin(), 1));
            }
            return callback(matchedCode, view.substr(sep + 1),
                            value + indexOffset, flag);
        }
        return true;
    };

    for (auto position : positions) {
        if (!trie.foreach(matchWord, position)) {
            return false;
        }
    }
    return true;
}

bool TableBasedDictionaryPrivate::matchTrie(
    std::string_view code, TableMatchMode mode, PhraseFlag flag,
    const TableMatchCallback &callback) const {
    const auto &trie = *trieByFlag(flag).first;
    if (!matchTrie(trie, 0, code, mode, flag, callback)) {
        return false;
    }

    if (flag == PhraseFlag::None) {
        unsigned int accumulatedIndex = phraseTrieIndex_;
        for (const auto &[trie, index] : extraTries_) {
            if (!matchTrie(trie, accumulatedIndex, code, mode, flag,
                           callback)) {
                return false;
            }
            accumulatedIndex += index;
        }
    }

    return true;
}

void TableBasedDictionaryPrivate::reset() {
    pinyinKey_ = promptKey_ = phraseKey_ = 0;
    phraseTrieIndex_ = userTrieIndex_ = 0;
    codeLength_ = 0;
    inputCode_.clear();
    ignoreChars_.clear();
    rules_.clear();
    rules_.shrink_to_fit();
    phraseTrie_.clear();
    singleCharTrie_.clear();
    singleCharConstTrie_.clear();
    singleCharLookupTrie_.clear();
    promptTrie_.clear();
    userTrie_.clear();
    autoPhraseDict_.clear();
    deletionTrie_.clear();
}
bool TableBasedDictionaryPrivate::validate() const {
    if (inputCode_.empty()) {
        return false;
    }
    if (inputCode_.count(pinyinKey_)) {
        return false;
    }
    if (inputCode_.count(promptKey_)) {
        return false;
    }
    if (inputCode_.count(phraseKey_)) {
        return false;
    }
    return true;
}

std::optional<std::tuple<std::string, std::string, PhraseFlag>>
TableBasedDictionaryPrivate::parseDataLine(std::string_view buf, bool user) {
    uint32_t special[3] = {pinyinKey_, phraseKey_, promptKey_};
    PhraseFlag specialFlag[] = {PhraseFlag::Pinyin, PhraseFlag::ConstructPhrase,
                                PhraseFlag::Prompt};
    auto spacePos = buf.find_first_of(" \n\r\f\v\t");
    if (spacePos == std::string::npos || spacePos + 1 == buf.size()) {
        return {};
    }
    auto wordPos = buf.find_first_not_of(" \n\r\f\v\t", spacePos);
    if (spacePos == std::string::npos || spacePos + 1 == buf.size()) {
        return {};
    }

    auto key = std::string_view(buf).substr(0, spacePos);
    std::string value{std::string_view(buf).substr(wordPos)};
    maybeUnescapeValue(value);

    if (key.empty() || value.empty()) {
        return {};
    }

    uint32_t firstChar;
    std::string_view::iterator next =
        fcitx::utf8::getNextChar(key.begin(), key.end(), &firstChar);
    auto *iter = std::find(std::begin(special), std::end(special), firstChar);
    PhraseFlag flag = user ? PhraseFlag::User : PhraseFlag::None;
    if (iter != std::end(special)) {
        // Reject flag for user.
        if (user) {
            return {};
        }
        flag = specialFlag[iter - std::begin(special)];
        key = key.substr(std::distance(key.begin(), next));
    }

    return std::tuple<std::string, std::string, PhraseFlag>{
        key, std::move(value), flag};
}

void TableBasedDictionaryPrivate::insertDataLine(std::string_view buf,
                                                 bool user) {
    if (auto data = parseDataLine(buf, user)) {
        auto &[key, value, flag] = *data;

        q_func()->insert(key, value, flag);
    }
}

bool TableBasedDictionaryPrivate::matchWordsInternal(
    std::string_view code, TableMatchMode mode, bool onlyChecking,
    const TableMatchCallback &callback) const {
    auto t0 = std::chrono::high_resolution_clock::now();

    // Match phrase trie.
    if (!matchTrie(code, mode, PhraseFlag::None,
                   [&callback, this](std::string_view code,
                                     std::string_view word, uint32_t index,
                                     PhraseFlag flag) {
                       if (!deletionTrie_.empty()) {
                           auto entry = generateTableEntry(code, word);
                           if (deletionTrie_.hasExactMatch(entry)) {
                               return true;
                           }
                       }
                       return callback(code, word, index, flag);
                   })) {
        return false;
    }

    LIBIME_TABLE_DEBUG() << "Match trie: " << millisecondsTill(t0);

    // Match Pinyin in the trie
    if (pinyinKey_) {
        auto pinyinCode = fcitx::stringutils::concat(
            fcitx::utf8::UCS4ToUTF8(pinyinKey_), code);
        // Apply following heuristic for pinyin.
        auto pinyinMode = TableMatchMode::Exact;
        int codeLength = fcitx::utf8::length(code);
        if (onlyChecking || codeLength >= options_.autoSelectLength() ||
            static_cast<size_t>(codeLength) > codeLength_ ||
            codeLength >= options_.noMatchAutoSelectLength()) {
            pinyinMode = TableMatchMode::Prefix;
        }
        if (!matchTrie(pinyinCode, pinyinMode, PhraseFlag::Pinyin, callback)) {
            return false;
        }
    }

    LIBIME_TABLE_DEBUG() << "Match pinyin: " << millisecondsTill(t0);

    // Match user phrase
    if (!matchTrie(code, mode, PhraseFlag::User, callback)) {
        return false;
    }

    LIBIME_TABLE_DEBUG() << "Match user: " << millisecondsTill(t0);
    auto matchAutoPhrase = [mode, code, &callback](std::string_view entry,
                                                   int32_t) {
        auto sep = entry.find(keyValueSeparator, code.size());
        if (sep == std::string::npos) {
            return true;
        }

        auto view = std::string_view(entry);
        auto matchedCode = view.substr(0, sep);
        if (mode == TableMatchMode::Prefix ||
            (mode == TableMatchMode::Exact &&
             fcitx::utf8::length(matchedCode) == fcitx::utf8::length(code))) {
            return callback(matchedCode, view.substr(sep + 1), 0,
                            PhraseFlag::Auto);
        }
        return true;
    };

    return autoPhraseDict_.search(code, matchAutoPhrase);
}

bool TableBasedDictionaryPrivate::validateHints(std::vector<std::string> &hints,
                                                const TableRule &rule) const {
    if (hints.size() <= 1) {
        return false;
    }

    for (const auto &ruleEntry : rule.entries()) {
        // skip rule entry like p00.
        if (ruleEntry.isPlaceHolder()) {
            continue;
        }

        if (ruleEntry.character() > hints.size()) {
            return false;
        }

        size_t index;
        if (ruleEntry.flag() == TableRuleEntryFlag::FromFront) {
            index = ruleEntry.character() - 1;
        } else {
            index = hints.size() - ruleEntry.character();
        }
        assert(index < hints.size());

        // Don't use hint for table with phrase key, or the requested length
        // longer.
        if (phraseKey_ ||
            fcitx::utf8::length(hints[index]) <
                static_cast<size_t>(std::abs(ruleEntry.index()))) {
            hints[index] = std::string();
        }
    }

    return true;
}

bool TableBasedDictionaryPrivate::hasExactMatchInPhraseTrie(
    std::string_view entry) const {
    return phraseTrie_.hasExactMatch(entry) ||
           std::any_of(extraTries_.begin(), extraTries_.end(),
                       [&entry](const auto &extraTrie) {
                           return extraTrie.first.hasExactMatch(entry);
                       });
}

void TableBasedDictionaryPrivate::loadBinary(std::istream &in) {
    FCITX_Q();
    throw_if_io_fail(unmarshall(in, pinyinKey_));
    throw_if_io_fail(unmarshall(in, promptKey_));
    throw_if_io_fail(unmarshall(in, phraseKey_));
    throw_if_io_fail(unmarshall(in, codeLength_));
    uint32_t size = 0;

    throw_if_io_fail(unmarshall(in, size));
    inputCode_.clear();
    while (size--) {
        uint32_t c;
        throw_if_io_fail(unmarshall(in, c));
        inputCode_.insert(c);
    }

    throw_if_io_fail(unmarshall(in, size));
    ignoreChars_.clear();
    while (size--) {
        uint32_t c;
        throw_if_io_fail(unmarshall(in, c));
        ignoreChars_.insert(c);
    }

    throw_if_io_fail(unmarshall(in, size));
    rules_.clear();
    while (size--) {
        rules_.emplace_back(in);
    }
    phraseTrie_ = decltype(phraseTrie_)(in);
    phraseTrieIndex_ = maxValue(phraseTrie_);
    singleCharTrie_ = decltype(singleCharTrie_)(in);
    if (q->hasRule()) {
        singleCharConstTrie_ = decltype(singleCharConstTrie_)(in);
        singleCharLookupTrie_ = decltype(singleCharLookupTrie_)(in);
    }
    if (promptKey_) {
        promptTrie_ = decltype(promptTrie_)(in);
    }
}

void TableBasedDictionaryPrivate::loadUserBinary(std::istream &in,
                                                 uint32_t version) {
    userTrie_ = decltype(userTrie_)(in);
    userTrieIndex_ = maxValue(userTrie_);
    autoPhraseDict_ = decltype(autoPhraseDict_)(TABLE_AUTOPHRASE_SIZE, in);
    // Version 2 introduced new deletion trie.
    if (version >= 2) {
        deletionTrie_ = decltype(deletionTrie_)(in);
    } else {
        deletionTrie_ = decltype(deletionTrie_)();
    }
}

TableBasedDictionary::TableBasedDictionary()
    : d_ptr(std::make_unique<TableBasedDictionaryPrivate>(this)) {
    FCITX_D();
    d->reset();
}

TableBasedDictionary::~TableBasedDictionary() = default;

void TableBasedDictionary::load(const char *filename, TableFormat format) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    throw_if_io_fail(in);
    load(in, format);
}

void TableBasedDictionary::load(std::istream &in, TableFormat format) {
    switch (format) {
    case TableFormat::Binary:
        loadBinary(in);
        break;
    case TableFormat::Text:
        loadText(in);
        break;
    default:
        throw std::invalid_argument("unknown format type");
    }
}

void TableBasedDictionary::loadText(std::istream &in) {
    FCITX_D();
    d->reset();

    std::string buf;

    auto check_option = [&buf](int index) {
        if (buf.compare(0, std::strlen(strConst[0][index]),
                        strConst[0][index]) == 0) {
            return 0;
        }
        if (buf.compare(0, std::strlen(strConst[1][index]),
                        strConst[1][index]) == 0) {
            return 1;
        }
        return -1;
    };

    auto isSpaceCheck = boost::is_any_of(" \n\t\r\v\f");
    auto phase = BuildPhase::PhaseConfig;
    while (!in.eof()) {
        if (!std::getline(in, buf)) {
            break;
        }

        // Validate everything first, so it's easier to process.
        if (!fcitx::utf8::validate(buf)) {
            continue;
        }

        boost::trim_if(buf, isSpaceCheck);

        switch (phase) {
        case BuildPhase::PhaseConfig: {
            if (buf[0] == '#') {
                continue;
            }

            int match = -1;
            if ((match = check_option(STR_KEYCODE)) >= 0) {
                const std::string code =
                    buf.substr(strlen(strConst[match][STR_KEYCODE]));
                auto range = fcitx::utf8::MakeUTF8CharRange(code);
                d->inputCode_ = std::set<uint32_t>(range.begin(), range.end());
            } else if ((match = check_option(STR_CODELEN)) >= 0) {
                d->codeLength_ =
                    std::stoi(buf.substr(strlen(strConst[match][STR_CODELEN])));
            } else if (check_option(STR_PINYINLEN) >= 0) {
                // Deprecated option.
            } else if ((match = check_option(STR_IGNORECHAR)) >= 0) {
                const std::string ignoreChars =
                    buf.substr(strlen(strConst[match][STR_IGNORECHAR]));
                auto range = fcitx::utf8::MakeUTF8CharRange(ignoreChars);
                d->ignoreChars_ =
                    std::set<uint32_t>(range.begin(), range.end());
            } else if ((match = check_option(STR_PINYIN)) >= 0) {
                d->pinyinKey_ = buf[strlen(strConst[match][STR_PINYIN])];
            } else if ((match = check_option(STR_PROMPT)) >= 0) {
                d->promptKey_ = buf[strlen(strConst[match][STR_PROMPT])];
            } else if ((match = check_option(STR_CONSTRUCTPHRASE)) >= 0) {
                d->phraseKey_ =
                    buf[strlen(strConst[match][STR_CONSTRUCTPHRASE])];
            } else if (check_option(STR_DATA) >= 0) {
                phase = BuildPhase::PhaseData;
                if (!d->validate()) {
                    throw std::invalid_argument("file format is invalid");
                }
                break;
            } else if (check_option(STR_RULE) >= 0) {
                phase = BuildPhase::PhaseRule;
                break;
            }
            break;
        }
        case BuildPhase::PhaseRule: {
            if (buf[0] == '#') {
                continue;
            }
            if (check_option(STR_DATA) >= 0) {
                phase = BuildPhase::PhaseData;
                if (!d->validate()) {
                    throw std::invalid_argument("file format is invalid");
                }
                break;
            }

            if (buf.empty()) {
                continue;
            }

            d->rules_.emplace_back(buf, d->codeLength_);
            break;
        }
        case BuildPhase::PhaseData:
            if (check_option(STR_PHRASE) >= 0) {
                phase = BuildPhase::PhasePhrase;
                if (!hasRule()) {
                    throw std::invalid_argument(
                        "file has phrase section but no rule");
                }
            }
            d->insertDataLine(buf, false);
            break;
        case BuildPhase::PhasePhrase:
            maybeUnescapeValue(buf);
            insert(buf, PhraseFlag::None);
            break;
        }
    }

    if (phase != BuildPhase::PhaseData && phase != BuildPhase::PhasePhrase) {
        throw_if_fail(in.bad(), std::ios_base::failure("io failed"));
        throw std::invalid_argument("file format is invalid");
    }
}

void TableBasedDictionary::saveText(std::ostream &out) {
    FCITX_D();
    out << strConst[1][STR_KEYCODE];
    for (auto c : d->inputCode_) {
        out << fcitx::utf8::UCS4ToUTF8(c);
    }
    out << '\n';
    out << strConst[1][STR_CODELEN] << d->codeLength_ << '\n';
    if (!d->ignoreChars_.empty()) {
        out << strConst[1][STR_IGNORECHAR];
        for (auto c : d->ignoreChars_) {
            out << fcitx::utf8::UCS4ToUTF8(c);
        }
        out << '\n';
    }
    if (d->pinyinKey_) {
        out << strConst[1][STR_PINYIN] << fcitx::utf8::UCS4ToUTF8(d->pinyinKey_)
            << '\n';
    }
    if (d->promptKey_) {
        out << strConst[1][STR_PROMPT] << fcitx::utf8::UCS4ToUTF8(d->promptKey_)
            << '\n';
    }
    if (d->phraseKey_) {
        out << strConst[1][STR_CONSTRUCTPHRASE]
            << fcitx::utf8::UCS4ToUTF8(d->phraseKey_) << '\n';
    }

    if (hasRule()) {
        out << strConst[1][STR_RULE] << '\n';
        for (const auto &rule : d->rules_) {
            out << rule.toString() << '\n';
        }
    }
    out << strConst[1][STR_DATA] << '\n';
    std::string buf;
    if (d->promptKey_) {
        auto promptString = fcitx::utf8::UCS4ToUTF8(d->promptKey_);
        d->promptTrie_.foreach(
            [&promptString, d, &buf,
             &out](uint32_t, size_t _len, DATrie<uint32_t>::position_type pos) {
                d->promptTrie_.suffix(buf, _len, pos);
                auto sep = buf.find(keyValueSeparator);
                if (sep == std::string::npos) {
                    return true;
                }
                std::string_view ref(buf);
                out << promptString << ref.substr(sep + 1) << " "
                    << maybeEscapeValue(ref.substr(0, sep)) << '\n';
                return true;
            });
    }
    if (d->phraseKey_) {
        auto phraseString = fcitx::utf8::UCS4ToUTF8(d->phraseKey_);
        d->singleCharConstTrie_.foreach(
            [&phraseString, d, &buf, &out](int32_t, size_t _len,
                                           DATrie<int32_t>::position_type pos) {
                d->singleCharConstTrie_.suffix(buf, _len, pos);
                auto sep = buf.find(keyValueSeparator);
                if (sep == std::string::npos) {
                    return true;
                }
                std::string_view ref(buf);
                out << phraseString << ref.substr(sep + 1) << " "
                    << maybeEscapeValue(ref.substr(0, sep)) << '\n';
                return true;
            });
    }

    saveTrieToText(d->phraseTrie_, out);
}

void TableBasedDictionary::loadBinary(std::istream &in) {
    FCITX_D();
    uint32_t magic;
    uint32_t version;
    throw_if_io_fail(unmarshall(in, magic));
    if (magic != tableBinaryFormatMagic) {
        throw std::invalid_argument("Invalid table magic.");
    }
    throw_if_io_fail(unmarshall(in, version));
    switch (version) {
    case 1:
        d->loadBinary(in);
        break;
    case tableBinaryFormatVersion:
        readZSTDCompressed(
            in, [d](std::istream &compressIn) { d->loadBinary(compressIn); });
        break;

    default:
        throw std::invalid_argument("Invalid table version.");
    }
}

void TableBasedDictionary::save(const char *filename, TableFormat format) {
    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    throw_if_io_fail(fout);
    save(fout, format);
}

void TableBasedDictionary::save(std::ostream &out, TableFormat format) {
    switch (format) {
    case TableFormat::Binary:
        saveBinary(out);
        break;
    case TableFormat::Text:
        saveText(out);
        break;
    default:
        throw std::invalid_argument("unknown format type");
    }
}

void TableBasedDictionary::saveBinary(std::ostream &origOut) {
    throw_if_io_fail(marshall(origOut, tableBinaryFormatMagic));
    throw_if_io_fail(marshall(origOut, tableBinaryFormatVersion));

    writeZSTDCompressed(origOut, [this](std::ostream &out) {
        FCITX_D();
        throw_if_io_fail(marshall(out, d->pinyinKey_));
        throw_if_io_fail(marshall(out, d->promptKey_));
        throw_if_io_fail(marshall(out, d->phraseKey_));
        throw_if_io_fail(marshall(out, d->codeLength_));
        throw_if_io_fail(
            marshall(out, static_cast<uint32_t>(d->inputCode_.size())));
        for (auto c : d->inputCode_) {
            throw_if_io_fail(marshall(out, c));
        }
        throw_if_io_fail(
            marshall(out, static_cast<uint32_t>(d->ignoreChars_.size())));
        for (auto c : d->ignoreChars_) {
            throw_if_io_fail(marshall(out, c));
        }
        throw_if_io_fail(
            marshall(out, static_cast<uint32_t>(d->rules_.size())));
        for (const auto &rule : d->rules_) {
            throw_if_io_fail(out << rule);
        }
        d->phraseTrie_.save(out);
        d->singleCharTrie_.save(out);
        if (hasRule()) {
            d->singleCharConstTrie_.save(out);
            d->singleCharLookupTrie_.save(out);
        }
        if (d->promptKey_) {
            d->promptTrie_.save(out);
        }
    });
}

void TableBasedDictionary::loadUser(const char *filename, TableFormat format) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    throw_if_io_fail(in);
    loadUser(in, format);
}

void TableBasedDictionary::loadUser(std::istream &in, TableFormat format) {
    FCITX_D();
    uint32_t magic = 0;
    uint32_t version = 0;
    switch (format) {
    case TableFormat::Binary:
        throw_if_io_fail(unmarshall(in, magic));
        if (magic != userTableBinaryFormatMagic) {
            throw std::invalid_argument("Invalid user table magic.");
        }
        throw_if_io_fail(unmarshall(in, version));
        switch (version) {
        case 1:
        case 2:
            d->loadUserBinary(in, version);
            break;
        case userTableBinaryFormatVersion:
            readZSTDCompressed(in, [d, version](std::istream &compressIn) {
                d->loadUserBinary(compressIn, version);
            });
            break;
        default:
            throw std::invalid_argument("Invalid user table version.");
        }
        break;
    case TableFormat::Text: {
        std::string buf;
        auto isSpaceCheck = boost::is_any_of(" \n\t\r\v\f");
        enum class UserDictState { Phrase, Auto, Delete };

        UserDictState state = UserDictState::Phrase;
        while (!in.eof()) {
            if (!std::getline(in, buf)) {
                break;
            }

            // Validate everything first, so it's easier to process.
            if (!fcitx::utf8::validate(buf)) {
                continue;
            }
            boost::trim_if(buf, isSpaceCheck);
            if (buf == UserDictAutoMark) {
                state = UserDictState::Auto;
                continue;
            }
            if (buf == UserDictDeleteMark) {
                state = UserDictState::Delete;
                continue;
            }

            switch (state) {
            case UserDictState::Phrase:
                d->insertDataLine(buf, true);
                break;
            case UserDictState::Auto: {
                auto tokens = fcitx::stringutils::split(buf, " \n\t\r\v\f");
                if (tokens.size() != 3 || !isAllInputCode(tokens[0])) {
                    continue;
                }
                try {
                    maybeUnescapeValue(tokens[1]);
                    int32_t hit = std::stoi(tokens[2]);
                    d->autoPhraseDict_.insert(
                        generateTableEntry(tokens[0], tokens[1]), hit);
                } catch (const std::exception &) {
                    continue;
                }
            } break;
            case UserDictState::Delete: {
                if (auto data = d->parseDataLine(buf, true)) {
                    auto &[key, value, flag] = *data;
                    auto entry = generateTableEntry(key, value);
                    d->deletionTrie_.set(entry, 0);
                }
            } break;
            }
        }
        break;
    }
    default:
        throw std::invalid_argument("unknown format type");
    }
}

void TableBasedDictionary::saveUser(const char *filename, TableFormat format) {
    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    throw_if_io_fail(fout);
    saveUser(fout, format);
}

void TableBasedDictionary::saveUser(std::ostream &out, TableFormat format) {
    FCITX_D();
    switch (format) {
    case TableFormat::Binary: {
        throw_if_io_fail(marshall(out, userTableBinaryFormatMagic));
        throw_if_io_fail(marshall(out, userTableBinaryFormatVersion));

        writeZSTDCompressed(out, [d](std::ostream &compressOut) {
            d->userTrie_.save(compressOut);
            throw_if_io_fail(compressOut);
            d->autoPhraseDict_.save(compressOut);
            throw_if_io_fail(compressOut);
            d->deletionTrie_.save(compressOut);
            throw_if_io_fail(compressOut);
        });
        break;
    }
    case TableFormat::Text: {
        saveTrieToText(d->userTrie_, out);

        if (!d->autoPhraseDict_.empty()) {
            out << UserDictAutoMark << '\n';
            std::vector<std::tuple<std::string, std::string, int32_t>>
                autoEntries;
            d->autoPhraseDict_.search(
                "", [&autoEntries](std::string_view entry, int hit) {
                    auto sep = entry.find(keyValueSeparator);
                    autoEntries.emplace_back(entry.substr(0, sep),
                                             entry.substr(sep + 1), hit);
                    return true;
                });
            for (auto &t : autoEntries | boost::adaptors::reversed) {
                out << std::get<0>(t) << " " << maybeEscapeValue(std::get<1>(t))
                    << " " << std::get<2>(t) << '\n';
            }
        }
        if (!d->deletionTrie_.empty()) {
            out << UserDictDeleteMark << '\n';
            saveTrieToText(d->deletionTrie_, out);
        }
        break;
    }
    default:
        throw std::invalid_argument("unknown format type");
    }
}

size_t TableBasedDictionary::loadExtra(const char *filename,
                                       TableFormat format) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    throw_if_io_fail(in);
    return loadExtra(in, format);
}

size_t TableBasedDictionary::loadExtra(std::istream &in, TableFormat format) {
    FCITX_D();
    DATrie<uint32_t> trie;
    uint32_t index = 0;
    uint32_t magic = 0;
    uint32_t version = 0;
    switch (format) {
    case TableFormat::Binary:
        throw_if_io_fail(unmarshall(in, magic));
        if (magic != extraTableBinaryFormatMagic) {
            throw std::invalid_argument("Invalid user table magic.");
        }
        throw_if_io_fail(unmarshall(in, version));
        switch (version) {
        case extraTableBinaryFormatVersion:
            readZSTDCompressed(in, [&trie](std::istream &compressIn) {
                trie.load(compressIn);
            });
            index = maxValue(trie);
            break;
        default:
            throw std::invalid_argument("Invalid user table version.");
        }
        break;
    case TableFormat::Text: {
        std::string buf;
        auto isSpaceCheck = boost::is_any_of(" \n\t\r\v\f");
        enum class ExtraDictState { Data, Phrase };

        ExtraDictState state = ExtraDictState::Data;
        while (!in.eof()) {
            if (!std::getline(in, buf)) {
                break;
            }

            // Validate everything first, so it's easier to process.
            if (!fcitx::utf8::validate(buf)) {
                continue;
            }
            boost::trim_if(buf, isSpaceCheck);
            if (buf == strConst[0][STR_PHRASE] ||
                buf == strConst[1][STR_PHRASE]) {
                state = ExtraDictState::Phrase;
                continue;
            }

            std::string key;
            std::string value;
            PhraseFlag flag;
            switch (state) {
            case ExtraDictState::Data:
                if (auto data = d->parseDataLine(buf, false)) {
                    std::tie(key, value, flag) = *data;
                    if (flag != PhraseFlag::None) {
                        continue;
                    }
                    maybeUnescapeValue(value);
                }
                break;
            case ExtraDictState::Phrase:
                value = buf;
                maybeUnescapeValue(value);
                if (!generate(value, key)) {
                    continue;
                }
                break;
            }
            if (value.empty() || key.empty()) {
                continue;
            }
            auto entry = generateTableEntry(key, value);
            insertOrUpdateTrie(trie, index, entry, false);
        }
        break;
    }
    default:
        throw std::invalid_argument("unknown format type");
    }

    d->extraTries_.push_back(std::make_pair(std::move(trie), index));
    return d->extraTries_.size() - 1;
}

void TableBasedDictionary::saveExtra(size_t index, const char *filename,
                                     TableFormat format) {
    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    throw_if_io_fail(fout);
    saveExtra(index, fout, format);
}

void TableBasedDictionary::saveExtra(size_t index, std::ostream &out,
                                     TableFormat format) {
    FCITX_D();
    if (index >= d->extraTries_.size()) {
        throw std::invalid_argument("Invalid extra dict index");
    }
    switch (format) {
    case TableFormat::Binary:

        throw_if_io_fail(marshall(out, extraTableBinaryFormatMagic));
        throw_if_io_fail(marshall(out, extraTableBinaryFormatVersion));

        writeZSTDCompressed(out, [d, index](std::ostream &compressOut) {
            d->extraTries_[index].first.save(compressOut);
        });
        break;
    case TableFormat::Text:
        saveTrieToText(d->extraTries_[index].first, out);
        break;
    default:
        throw std::invalid_argument("unknown format type");
    }
}

void TableBasedDictionary::removeAllExtra() {
    FCITX_D();
    d->extraTries_.clear();
}

bool TableBasedDictionary::hasRule() const noexcept {
    FCITX_D();
    return !d->rules_.empty();
}

bool TableBasedDictionary::hasCustomPrompt() const noexcept {
    FCITX_D();
    return !d->promptTrie_.empty();
}

const TableRule *TableBasedDictionary::findRule(std::string_view name) const {
    FCITX_D();
    for (const auto &rule : d->rules_) {
        if (rule.name() == name) {
            return &rule;
        }
    }
    return nullptr;
}

bool TableBasedDictionary::insert(std::string_view value,
                                  libime::PhraseFlag flag) {
    std::string key;
    if (flag != PhraseFlag::None && flag != PhraseFlag::User) {
        return false;
    }

    if (generate(value, key)) {
        return insert(key, value, flag);
    }
    return false;
}

bool TableBasedDictionary::insert(std::string_view key, std::string_view value,
                                  PhraseFlag flag, bool verifyWithRule) {
    FCITX_D();

    if (!d->validateKeyValue(key, value, flag)) {
        return false;
    }

    switch (flag) {
    case PhraseFlag::Pinyin: /* Falls through. */
    case PhraseFlag::User:   /* Falls through. */
    case PhraseFlag::None: {
        if (flag != PhraseFlag::Pinyin && verifyWithRule && hasRule()) {
            std::string checkKey;
            if (!generate(value, checkKey)) {
                return false;
            }
            if (checkKey != key) {
                return false;
            }
        }

        if (!d->insert(key, value, flag)) {
            return false;
        }

        if (flag == PhraseFlag::None && fcitx::utf8::length(value) == 1 &&
            !d->ignoreChars_.count(fcitx::utf8::getChar(value))) {
            updateReverseLookupEntry(d->singleCharTrie_, key, value, nullptr);

            if (hasRule() && !d->phraseKey_) {
                updateReverseLookupEntry(d->singleCharConstTrie_, key, value,
                                         &d->singleCharLookupTrie_);
            }
        }
        break;
    }
    case PhraseFlag::Prompt:
        if (!key.empty()) {
            d->promptTrie_.set(generateTableEntry(key, value), 0);
        } else {
            return false;
        }
        break;
    case PhraseFlag::ConstructPhrase:
        if (hasRule() && fcitx::utf8::length(value) == 1) {
            updateReverseLookupEntry(d->singleCharConstTrie_, key, value,
                                     &d->singleCharLookupTrie_);
        }
        break;
    case PhraseFlag::Auto: {
        const auto entry = generateTableEntry(key, value);
        auto hit = d->autoPhraseDict_.exactSearch(entry);
        if (tableOptions().saveAutoPhraseAfter() >= 1 &&
            static_cast<uint32_t>(tableOptions().saveAutoPhraseAfter()) <=
                hit + 1) {
            d->autoPhraseDict_.erase(entry);
            insert(key, value, PhraseFlag::User, false);
        } else {
            d->autoPhraseDict_.insert(entry);
        }
    } break;
    case PhraseFlag::Invalid:
        break;
    }
    return true;
}

bool TableBasedDictionary::generate(std::string_view value,
                                    std::string &key) const {
    return generateWithHint(value, {}, key);
}

bool TableBasedDictionary::generateWithHint(
    std::string_view value, const std::vector<std::string> &codeHints,
    std::string &key) const {
    FCITX_D();
    if (!hasRule() || value.empty()) {
        return false;
    }

    // Check word is valid utf8.
    auto valueLen = fcitx::utf8::lengthValidated(value);
    if (valueLen == fcitx::utf8::INVALID_LENGTH) {
        return false;
    }

    for (const auto &code : codeHints) {
        if (!fcitx::utf8::validate(code)) {
            return false;
        }
    }

    auto hints = codeHints;
    hints.resize(valueLen);

    std::string newKey;
    for (const auto &rule : d->rules_) {
        // check rule can be applied
        const bool canApplyRule =
            ((rule.flag() == TableRuleFlag::LengthEqual &&
              valueLen == rule.phraseLength()) ||
             (rule.flag() == TableRuleFlag::LengthLongerThan &&
              valueLen >= rule.phraseLength()));
        if (!canApplyRule) {
            continue;
        }

        auto hints = codeHints;
        hints.resize(valueLen);
        // Fill hints first.
        if (!d->validateHints(hints, rule)) {
            continue;
        }

        bool success = true;
        std::set<std::pair<size_t, int>> usedChar;
        for (const auto &ruleEntry : rule.entries()) {
            std::string_view::const_iterator iter;
            // skip rule entry like p00.
            if (ruleEntry.isPlaceHolder()) {
                continue;
            }

            if (ruleEntry.character() > valueLen) {
                success = false;
                break;
            }

            size_t index;
            if (ruleEntry.flag() == TableRuleEntryFlag::FromFront) {
                index = ruleEntry.character() - 1;
            } else {
                index = valueLen - ruleEntry.character();
            }
            iter = fcitx::utf8::nextNChar(value.begin(), index);
            std::string_view::iterator prev = iter;
            iter = fcitx::utf8::nextChar(iter);
            std::string_view chr(&*prev, std::distance(prev, iter));

            std::string entry;
            if (!hints[index].empty()) {
                entry = hints[index];
            } else {
                entry = reverseLookup(chr, PhraseFlag::ConstructPhrase);
            }
            if (entry.empty()) {
                success = false;
                break;
            }

            auto length = fcitx::utf8::lengthValidated(entry);
            auto codeIndex = ruleEntry.index();
            if (length == fcitx::utf8::INVALID_LENGTH ||
                length < static_cast<size_t>(std::abs(codeIndex))) {
                continue;
            }

            if (codeIndex > 0) {
                // code index starts with 1.
                codeIndex -= 1;
            } else {
                codeIndex = static_cast<int>(length) + codeIndex;
            }

            auto charIndex = std::make_pair(index, codeIndex);
            // Avoid same code being referenced twice.
            // This helps for the case like: p11 and p1z point to the same code
            // character.
            if (usedChar.count(charIndex)) {
                continue;
            }
            usedChar.insert(charIndex);
            auto entryStart = fcitx::utf8::nextNChar(entry.begin(), codeIndex);
            auto entryEnd = fcitx::utf8::nextChar(entryStart);

            newKey.append(entryStart, entryEnd);
        }

        if (success && !newKey.empty()) {
            key = newKey;
            return true;
        }
    }

    return false;
}

bool TableBasedDictionary::isInputCode(uint32_t c) const {
    FCITX_D();
    return !!(d->inputCode_.count(c));
}

bool TableBasedDictionary::isAllInputCode(std::string_view code) const {
    std::string_view::iterator iter = code.begin();
    std::string_view::iterator end = code.end();
    while (iter != end) {
        uint32_t chr;
        iter = fcitx::utf8::getNextChar(iter, end, &chr);
        if (!fcitx::utf8::isValidChar(chr) || !isInputCode(chr)) {
            return false;
        }
    }
    return true;
}

bool TableBasedDictionary::isEndKey(uint32_t c) const {
    FCITX_D();
    return !!d->options_.endKey().count(c);
}

void TableBasedDictionary::statistic() const {
    FCITX_D();
    std::cout << "Phrase Trie: " << d->phraseTrie_.mem_size() << '\n'
              << "Single Char Trie: " << d->singleCharTrie_.mem_size() << '\n'
              << "Single char const trie: "
              << d->singleCharConstTrie_.mem_size() << " + "
              << d->singleCharLookupTrie_.mem_size() << '\n'
              << "Prompt Trie: " << d->promptTrie_.mem_size() << '\n';
}

void TableBasedDictionary::setTableOptions(TableOptions option) {
    FCITX_D();
    d->options_ = std::move(option);
    if (d->options_.autoSelectLength() < 0) {
        d->options_.setAutoSelectLength(maxLength());
    }
    if (d->options_.noMatchAutoSelectLength() < 0) {
        d->options_.setNoMatchAutoSelectLength(maxLength());
    }
    if (d->options_.autoPhraseLength() < 0) {
        d->options_.setAutoPhraseLength(maxLength());
    }
    d->autoSelectRegex_.reset();
    d->noMatchAutoSelectRegex_.reset();
    try {
        if (!d->options_.autoSelectRegex().empty()) {
            d->autoSelectRegex_.emplace(d->options_.autoSelectRegex());
        }
    } catch (...) {
    }
    try {
        if (!d->options_.noMatchAutoSelectRegex().empty()) {
            d->noMatchAutoSelectRegex_.emplace(
                d->options_.noMatchAutoSelectRegex());
        }
    } catch (...) {
    }
}

const TableOptions &TableBasedDictionary::tableOptions() const {
    FCITX_D();
    return d->options_;
}

bool TableBasedDictionary::hasPinyin() const {
    FCITX_D();
    return d->pinyinKey_;
}

uint32_t TableBasedDictionary::maxLength() const {
    FCITX_D();
    return d->codeLength_;
}

bool TableBasedDictionary::isValidLength(size_t length) const {
    FCITX_D();
    return length <= d->codeLength_;
}
bool TableBasedDictionary::matchWords(
    std::string_view code, TableMatchMode mode,
    const TableMatchCallback &callback) const {
    FCITX_D();
    return d->matchWordsInternal(code, mode, false, callback);
}

bool TableBasedDictionary::hasMatchingWords(std::string_view code,
                                            std::string_view next) const {
    std::string str{code};
    str.append(next.data(), next.size());
    return hasMatchingWords(str);
}

bool TableBasedDictionary::hasMatchingWords(std::string_view code) const {
    FCITX_D();
    bool hasMatch = false;
    d->matchWordsInternal(
        code, TableMatchMode::Prefix, true,
        [&hasMatch](std::string_view, std::string_view, uint32_t, PhraseFlag) {
            hasMatch = true;
            return false;
        });
    return hasMatch;
}

bool TableBasedDictionary::hasOneMatchingWord(std::string_view code) const {
    // User dict may have the same entry, so we need to check if it is the same.
    std::optional<std::tuple<std::string, std::string>> previousMatch;
    matchWords(code, TableMatchMode::Prefix,
               [&previousMatch](std::string_view code, std::string_view word,
                                uint32_t, PhraseFlag) {
                   if (previousMatch) {
                       if (std::get<0>(*previousMatch) == code &&
                           std::get<1>(*previousMatch) == word) {
                           return true;
                       }
                       previousMatch.reset();
                       return false;
                   }
                   previousMatch.emplace(code, word);
                   return true;
               });
    return previousMatch.has_value();
}

PhraseFlag TableBasedDictionary::wordExists(std::string_view code,
                                            std::string_view word) const {
    FCITX_D();
    auto entry = generateTableEntry(code, word);

    if (d->userTrie_.hasExactMatch(entry)) {
        return PhraseFlag::User;
    }
    if (d->hasExactMatchInPhraseTrie(entry) &&
        !d->deletionTrie_.hasExactMatch(entry)) {
        return PhraseFlag::None;
    }

    if (d->autoPhraseDict_.exactSearch(entry)) {
        return PhraseFlag::Auto;
    }
    return PhraseFlag::Invalid;
}

void TableBasedDictionary::removeWord(std::string_view code,
                                      std::string_view word) {
    FCITX_D();
    auto entry = generateTableEntry(code, word);
    d->autoPhraseDict_.erase(entry);
    d->userTrie_.erase(entry);
    if (d->hasExactMatchInPhraseTrie(entry) &&
        !d->deletionTrie_.hasExactMatch(entry)) {
        d->deletionTrie_.set(entry, 0);
    }
}

std::string TableBasedDictionary::reverseLookup(std::string_view word,
                                                PhraseFlag flag) const {
    FCITX_D();
    if (flag != PhraseFlag::ConstructPhrase && flag != PhraseFlag::None) {
        throw std::runtime_error("Invalid flag.");
    }
    std::string reverseEntry{word};
    reverseEntry.push_back(keyValueSeparator);
    std::string key;
    const auto &trie =
        (flag == PhraseFlag::ConstructPhrase ? d->singleCharConstTrie_
                                             : d->singleCharTrie_);
    trie.foreach(
        reverseEntry,
        [&trie, &key](int32_t, size_t len, DATrie<int32_t>::position_type pos) {
            trie.suffix(key, len, pos);
            return false;
        });
    return key;
}

std::string TableBasedDictionary::hint(std::string_view key) const {
    FCITX_D();
    if (!d->promptKey_) {
        return std::string{key};
    }

    std::string result;
    auto range = fcitx::utf8::MakeUTF8CharRange(key);
    for (auto iter = std::begin(range); iter != std::end(range); iter++) {
        auto charRange = iter.charRange();
        std::string_view search(
            &*charRange.first,
            std::distance(charRange.first, charRange.second));
        std::string entry;
        d->promptTrie_.foreach(
            generateTableEntry(search, ""),
            [&entry, d](uint32_t, size_t len,
                        DATrie<uint32_t>::position_type pos) {
                d->promptTrie_.suffix(entry, len, pos);
                return false;
            });
        if (!entry.empty()) {
            result.append(entry);
        } else {
            result.append(charRange.first, charRange.second);
        }
    }
    return result;
}

void TableBasedDictionary::matchPrefixImpl(
    const SegmentGraph &graph, const GraphMatchCallback &callback,
    const std::unordered_set<const SegmentGraphNode *> &ignore,
    void * /*helper*/) const {
    FCITX_D();
    auto range = fcitx::utf8::MakeUTF8CharRange(graph.data());
    auto hasWildcard =
        d->options_.matchingKey() &&
        std::any_of(std::begin(range), std::end(range),
                    [d](uint32_t c) { return d->options_.matchingKey() == c; });

    const TableMatchMode mode = tableOptions().exactMatch() || hasWildcard
                                    ? TableMatchMode::Exact
                                    : TableMatchMode::Prefix;
    SegmentGraphPath path;
    path.reserve(2);
    graph.bfs(&graph.start(), [this, &ignore, &path, &callback, hasWildcard,
                               mode](const SegmentGraphBase &graph,
                                     const SegmentGraphNode *node) {
        if (!node->prevSize() || ignore.count(node)) {
            return true;
        }
        for (const auto &prev : node->prevs()) {
            path.clear();
            path.push_back(&prev);
            path.push_back(node);

            auto code = graph.segment(*path[0], *path[1]);
            if (code.size() == graph.size()) {
                matchWords(
                    code, mode,
                    [&](std::string_view code, std::string_view word,
                        uint32_t index, PhraseFlag flag) {
                        // Do not return user for noSortInputLength, so code
                        // shorter than noSortInputLength is always in stable
                        // order.
                        if (flag == PhraseFlag::User &&
                            code.size() <= tableOptions().noSortInputLength()) {
                            return true;
                        }

                        WordNode wordNode(word, InvalidWordIndex);

                        // for length 1 "pinyin", skip long pinyin as an
                        // optimization.
                        if (flag == PhraseFlag::Pinyin && graph.size() == 1 &&
                            code.size() != 1) {
                            return true;
                        }
                        callback(path, wordNode, 0,
                                 std::make_unique<TableLatticeNodePrivate>(
                                     code, index, flag));
                        return true;
                    });
            } else if (!hasWildcard) {
                // use it as a buffer.
                std::string entry;
                FCITX_D();
                d->singleCharLookupTrie_.foreach(
                    code, [&](uint32_t, size_t len,
                              DATrie<uint32_t>::position_type pos) {
                        d->singleCharLookupTrie_.suffix(entry,
                                                        code.size() + len, pos);

                        auto sep = entry.find(keyValueSeparator);
                        if (sep == std::string::npos) {
                            return true;
                        }

                        std::string_view ref(entry);
                        auto code = ref.substr(0, sep);
                        auto word = ref.substr(sep + 1);

                        WordNode wordNode(word, InvalidWordIndex);
                        callback(path, wordNode, 0,
                                 std::make_unique<TableLatticeNodePrivate>(
                                     code, 0, PhraseFlag::ConstructPhrase));
                        return true;
                    });
            }
        }
        return true;
    });
}
} // namespace libime
