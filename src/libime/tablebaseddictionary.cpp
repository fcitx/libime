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

#include "tablebaseddictionary.h"
#include "datrie.h"
#include "tablerule.h"
#include <boost/algorithm/string.hpp>
#include <cstring>
#include <fcitx-utils/utf8.h>
#include <fstream>
#include <set>
#include <string>

namespace libime {

static const char keyValueSeparator = '\x01';

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
    STR_LAST
};

enum class BuildPhase { PhaseConfig, PhaseRule, PhaseData } phase = BuildPhase::PhaseConfig;

static const char *strConst[2][STR_LAST] = {
    {"键码=", "码长=", "规避字符=", "拼音=", "拼音长度=", "[数据]", "[组词规则]", "提示=", "构词="},
    {"KeyCode=", "Length=", "InvalidChar=", "Pinyin=", "PinyinLength=", "[Data]", "[Rule]",
     "Prompt=", "ConstructPhrase="}};

class TableBasedDictionaryPrivate {
public:
    std::vector<TableRule> rules;
    std::set<char> inputCode;
    std::set<char> ignoreChars;
    char pinyinKey;
    char promptKey;
    char phraseKey;
    int32_t codeLength;
    DATrie<int32_t> phraseTrie;          // base dictionary
    DATrie<int32_t> singleCharTrie;      // reverse lookup from single character
    DATrie<int32_t> singleCharConstTrie; // lookup char for new phrase
    DATrie<int32_t> promptTrie;          // lookup for prompt;
    DATrie<int32_t> pinyinPhraseTrie;
    std::function<bool(char)> charCheck;
    TableBasedDictionaryPrivate() {
        charCheck = [this](char c) { return inputCode.find(c) != inputCode.end(); };
    }

    TableBasedDictionaryPrivate(const TableBasedDictionaryPrivate &other)
        : rules(other.rules), inputCode(other.inputCode), ignoreChars(other.ignoreChars),
          pinyinKey(other.pinyinKey), promptKey(other.promptKey), phraseKey(other.phraseKey),
          codeLength(other.codeLength), phraseTrie(other.phraseTrie),
          singleCharTrie(other.singleCharTrie), singleCharConstTrie(other.singleCharConstTrie),
          promptTrie(other.promptTrie), pinyinPhraseTrie(other.pinyinPhraseTrie) {
        charCheck = [this](char c) { return inputCode.find(c) != inputCode.end(); };
    }

    void reset() {
        pinyinKey = promptKey = phraseKey = '\0';
        codeLength = 0;
        inputCode.clear();
        ignoreChars.clear();
        rules.clear();
        rules.shrink_to_fit();
        phraseTrie.clear();
        singleCharTrie.clear();
        singleCharConstTrie.clear();
        promptTrie.clear();
        pinyinPhraseTrie.clear();
    }
    bool validate() {
        if (inputCode.empty()) {
            return false;
        }
        if (inputCode.find(pinyinKey) != inputCode.end()) {
            return false;
        }
        if (inputCode.find(promptKey) != inputCode.end()) {
            return false;
        }
        if (inputCode.find(phraseKey) != inputCode.end()) {
            return false;
        }
        return true;
    }
};

void updateReverseLookupEntry(DATrie<int32_t> &trie, boost::string_view key,
                              boost::string_view value) {
    auto reverseEntry = value.to_string() + keyValueSeparator;
    bool insert = true;
    trie.foreach (reverseEntry, [&trie, &key, &value, &insert, &reverseEntry](
                                    int32_t, size_t len, DATrie<int32_t>::position_type pos) {
        auto oldKeyLen = len;
        if (key.length() > oldKeyLen) {
            std::string oldEntry;
            trie.suffix(oldEntry, len, pos);
            trie.erase(pos);
        } else {
            insert = false;
        }
        return false;
    });
    if (insert) {
        reverseEntry.append(key.begin(), key.end());
        trie.set(reverseEntry, 1);
    }
}

TableBasedDictionary::TableBasedDictionary()
    : d_ptr(std::make_unique<TableBasedDictionaryPrivate>()) {
    FCITX_D();
    d->reset();
}

TableBasedDictionary::~TableBasedDictionary() {}

TableBasedDictionary::TableBasedDictionary(TableBasedDictionary &&other) noexcept
    : d_ptr(std::move(other.d_ptr)) {}

TableBasedDictionary::TableBasedDictionary(const libime::TableBasedDictionary &other)
    : d_ptr(std::make_unique<TableBasedDictionaryPrivate>(*other.d_ptr)) {}

TableBasedDictionary::TableBasedDictionary(const char *filename,
                                           TableBasedDictionary::TableFormat format)
    : TableBasedDictionary() {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    throw_if_io_fail(in);

    switch (format) {
    case TableFormat::Binary:
        open(in);
        break;
    case TableFormat::Text:
        build(in);
        break;
    default:
        throw std::invalid_argument("unknown format type");
    }
}

TableBasedDictionary::TableBasedDictionary(std::istream &in,
                                           TableBasedDictionary::TableFormat format)
    : TableBasedDictionary() {
    switch (format) {
    case TableFormat::Binary:
        open(in);
        break;
    case TableFormat::Text:
        build(in);
        break;
    default:
        throw std::invalid_argument("unknown format type");
    }
}

TableBasedDictionary &TableBasedDictionary::operator=(TableBasedDictionary other) {
    swap(*this, other);
    return *this;
}

void TableBasedDictionary::build(std::istream &in) {
    FCITX_D();
    d->reset();

    std::string buf;
    size_t lineNumber = 0;

    auto check_option = [&buf](int index) {
        if (buf.compare(0, std::strlen(strConst[0][index]), strConst[0][index]) == 0) {
            return 0;
        } else if (buf.compare(0, std::strlen(strConst[1][index]), strConst[1][index]) == 0) {
            return 1;
        }
        return -1;
    };

    auto isSpaceCheck = boost::is_any_of(" \n\t\r\v\f");
    while (!in.eof()) {
        if (!std::getline(in, buf)) {
            break;
        }
        lineNumber++;

        boost::trim_if(buf, isSpaceCheck);

        switch (phase) {
        case BuildPhase::PhaseConfig: {
            if (buf[0] == '#') {
                continue;
            }

            int match = -1;
            if ((match = check_option(STR_KEYCODE)) >= 0) {
                const std::string code = buf.substr(strlen(strConst[match][STR_KEYCODE]));
                d->inputCode = std::set<char>(code.begin(), code.end());
            } else if ((match = check_option(STR_CODELEN)) >= 0) {
                d->codeLength = std::stoi(buf.substr(strlen(strConst[match][STR_CODELEN])));
            } else if ((match = check_option(STR_IGNORECHAR)) >= 0) {
                const std::string ignoreChars = buf.substr(strlen(strConst[match][STR_KEYCODE]));
                d->ignoreChars = std::set<char>(ignoreChars.begin(), ignoreChars.end());
            } else if ((match = check_option(STR_PINYIN)) >= 0) {
                d->pinyinKey = buf[strlen(strConst[match][STR_PINYIN])];
            } else if ((match = check_option(STR_PROMPT)) >= 0) {
                d->promptKey = buf[strlen(strConst[match][STR_PROMPT])];
            } else if ((match = check_option(STR_CONSTRUCTPHRASE)) >= 0) {
                d->phraseKey = buf[strlen(strConst[match][STR_CONSTRUCTPHRASE])];
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

            d->rules.emplace_back(buf, d->codeLength);
        }
        case BuildPhase::PhaseData: {
            std::array<char, 3> special = {d->pinyinKey, d->phraseKey, d->promptKey};
            PhraseFlag specialFlag[] = {PhraseFlagPinyin, PhraseFlagConstructPhrase,
                                        PhraseFlagPrompt};
            auto spacePos = buf.find_first_of(" \n\r\f\v\t");
            if (spacePos == std::string::npos || spacePos + 1 == buf.size()) {
                continue;
            }
            auto wordPos = buf.find_first_not_of(" \n\r\f\v\t", spacePos);
            if (spacePos == std::string::npos || spacePos + 1 == buf.size()) {
                continue;
            }

            auto key = boost::string_view(buf).substr(0, spacePos);
            auto value = boost::string_view(buf).substr(wordPos);

            auto iter = std::find(special.begin(), special.end(), key[0]);
            PhraseFlag flag = PhraseFlagNone;
            if (iter != special.end()) {
                flag = specialFlag[iter - special.begin()];
                key = key.substr(1);
            }

            insert(key.to_string(), value.to_string(), flag);
        }
        }
    }

    if (phase != BuildPhase::PhaseData) {
        throw_if_fail(in.bad(), std::ios_base::failure("io failed"));
        throw std::invalid_argument("file format is invalid");
    }
}

void TableBasedDictionary::dump(const char *filename) {
    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    throw_if_io_fail(fout);
    dump(fout);
}

void TableBasedDictionary::dump(std::ostream &out) {
    FCITX_D();
    out << strConst[1][STR_KEYCODE];
    for (auto c : d->inputCode) {
        out << c;
    }
    out << std::endl;
    out << strConst[1][STR_CODELEN] << d->codeLength << std::endl;
    if (d->ignoreChars.size()) {
        out << strConst[1][STR_IGNORECHAR];
        for (auto c : d->ignoreChars) {
            out << c;
        }
        out << std::endl;
    }
    if (d->pinyinKey) {
        out << strConst[1][STR_PINYIN] << d->pinyinKey << std::endl;
    }
    if (d->promptKey) {
        out << strConst[1][STR_PROMPT] << d->promptKey << std::endl;
    }
    if (d->phraseKey) {
        out << strConst[1][STR_CONSTRUCTPHRASE] << d->phraseKey << std::endl;
    }

    if (hasRule()) {
        out << strConst[1][STR_RULE] << std::endl;
        for (const auto &rule : d->rules) {
            out << rule.toString() << std::endl;
        }
    }
    out << strConst[1][STR_DATA] << std::endl;
    std::string buf;
    if (d->promptKey) {
        d->promptTrie.foreach ([this, d, &buf, &out](int32_t, size_t _len,
                                                     DATrie<int32_t>::position_type pos) {
            d->promptTrie.suffix(buf, _len, pos);
            auto sep = buf.find(keyValueSeparator);
            boost::string_view ref(buf);
            out << d->promptKey << ref.substr(0, sep) << " " << ref.substr(sep + 1) << std::endl;
            return true;
        });
    }
    if (d->phraseKey) {
        d->singleCharConstTrie.foreach ([this, d, &buf, &out](int32_t, size_t _len,
                                                              DATrie<int32_t>::position_type pos) {
            d->singleCharConstTrie.suffix(buf, _len, pos);
            auto sep = buf.find(keyValueSeparator);
            boost::string_view ref(buf);
            out << d->promptKey << ref.substr(sep + 1) << " " << ref.substr(0, sep) << std::endl;
            return true;
        });
    }
    d->phraseTrie.foreach (
        [this, d, &buf, &out](int32_t, size_t _len, DATrie<int32_t>::position_type pos) {
            d->phraseTrie.suffix(buf, _len, pos);
            auto sep = buf.find(keyValueSeparator);
            boost::string_view ref(buf);
            out << ref.substr(0, sep) << " " << ref.substr(sep + 1) << std::endl;
            return true;
        });
    d->pinyinPhraseTrie.foreach (
        [this, d, &buf, &out](int32_t, size_t _len, DATrie<int32_t>::position_type pos) {
            d->pinyinPhraseTrie.suffix(buf, _len, pos);
            auto sep = buf.find(keyValueSeparator);
            boost::string_view ref(buf);
            out << d->pinyinKey << ref.substr(0, sep) << " " << ref.substr(sep + 1) << std::endl;
            return true;
        });
}

void TableBasedDictionary::open(std::istream &in) {
    FCITX_D();
    throw_if_io_fail(unmarshall(in, d->pinyinKey));
    throw_if_io_fail(unmarshall(in, d->promptKey));
    throw_if_io_fail(unmarshall(in, d->phraseKey));
    throw_if_io_fail(unmarshall(in, d->codeLength));
    uint32_t size;

    throw_if_io_fail(unmarshall(in, size));
    d->inputCode.clear();
    while (size--) {
        char c;
        throw_if_io_fail(unmarshall(in, c));
        d->inputCode.insert(c);
    }

    throw_if_io_fail(unmarshall(in, size));
    d->ignoreChars.clear();
    while (size--) {
        char c;
        throw_if_io_fail(unmarshall(in, c));
        d->ignoreChars.insert(c);
    }

    throw_if_io_fail(unmarshall(in, size));
    d->rules.clear();
    while (size--) {
        d->rules.emplace_back(in);
    }
    d->phraseTrie = decltype(d->phraseTrie)(in);
    d->singleCharTrie = decltype(d->singleCharTrie)(in);
    if (hasRule()) {
        d->singleCharConstTrie = decltype(d->singleCharConstTrie)(in);
    }
    if (d->promptKey) {
        d->promptTrie = decltype(d->promptTrie)(in);
    }
    if (d->pinyinKey) {
        d->pinyinPhraseTrie = decltype(d->pinyinPhraseTrie)(in);
    }
}

void TableBasedDictionary::save(const char *filename) {
    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    throw_if_io_fail(fout);
    save(fout);
}

void TableBasedDictionary::save(std::ostream &out) {
    FCITX_D();
    throw_if_io_fail(marshall(out, d->pinyinKey));
    throw_if_io_fail(marshall(out, d->promptKey));
    throw_if_io_fail(marshall(out, d->phraseKey));
    throw_if_io_fail(marshall(out, d->codeLength));
    throw_if_io_fail(marshall(out, static_cast<uint32_t>(d->inputCode.size())));
    for (auto c : d->inputCode) {
        throw_if_io_fail(marshall(out, c));
    }
    throw_if_io_fail(marshall(out, static_cast<uint32_t>(d->ignoreChars.size())));
    for (auto c : d->ignoreChars) {
        throw_if_io_fail(marshall(out, c));
    }
    throw_if_io_fail(marshall(out, static_cast<uint32_t>(d->rules.size())));
    for (const auto &rule : d->rules) {
        throw_if_io_fail(out << rule);
    }
    d->phraseTrie.save(out);
    d->singleCharTrie.save(out);
    if (hasRule()) {
        d->singleCharConstTrie.save(out);
    }
    if (d->promptKey) {
        d->promptTrie.save(out);
    }
    if (d->pinyinKey) {
        d->pinyinPhraseTrie.save(out);
    }
}

bool TableBasedDictionary::hasRule() const noexcept {
    FCITX_D();
    return !d->rules.empty();
}

bool TableBasedDictionary::insert(const std::string &value) {
    std::string key;
    if (generate(value, key)) {
        return insert(key, value);
    }
    return false;
}

bool TableBasedDictionary::insert(const std::string &key, const std::string &value, PhraseFlag flag,
                                  bool verifyWithRule) {
    FCITX_D();
    if (flag != PhraseFlagPinyin && !boost::all(key, d->charCheck)) {
        return false;
    }

    // invalid char
    if (!fcitx::utf8::validate(value.c_str())) {
        return false;
    }

    switch (flag) {
    case PhraseFlagNone: {
        if (verifyWithRule && hasRule()) {
            std::string checkKey;
            if (!generate(value, checkKey)) {
                return false;
            }
            if (checkKey != key) {
                return false;
            }
        }

        auto entry = key + keyValueSeparator + value;
        if (d->phraseTrie.exactMatchSearch(entry) > 0) {
            return false;
        }
        d->phraseTrie.set(entry, flag);

        if (fcitx_utf8_strnlen(value.c_str(), value.size()) == 1) {
            updateReverseLookupEntry(d->singleCharTrie, key, value);

            if (hasRule() && !d->phraseKey) {
                updateReverseLookupEntry(d->singleCharConstTrie, key, value);
            }
        }
        break;
    }
    case PhraseFlagPinyin: {
        auto entry = key + keyValueSeparator + value;
        if (d->pinyinPhraseTrie.exactMatchSearch(entry) > 0) {
            return false;
        }
        d->pinyinPhraseTrie.set(entry, flag);
        break;
    }
    case PhraseFlagPrompt:
        if (key.size() == 1) {
            auto promptChar = fcitx::utf8::getCharValidated(value);
            d->promptTrie.set(key, promptChar);
        } else {
            return false;
        }
        break;
    case PhraseFlagConstructPhrase:
        if (hasRule()) {
            updateReverseLookupEntry(d->singleCharConstTrie, key, value);
        }
        break;
    }
    return true;
}

bool TableBasedDictionary::generate(const std::string &value, std::string &key) {
    FCITX_D();
    if (!hasRule() || value.empty()) {
        return false;
    }

    if (!fcitx::utf8::validate(value)) {
        return false;
    }

    auto valueLen = fcitx::utf8::length(value);

    std::string newKey;
    std::string entry;
    for (const auto &rule : d->rules) {
        // check rule can be applied
        if (!((rule.flag == TableRuleFlag::LengthEqual && valueLen == rule.phraseLength) ||
              (rule.flag == TableRuleFlag::LengthLongerThan && valueLen >= rule.phraseLength))) {
            continue;
        }

        bool success = true;
        for (const auto &ruleEntry : rule.entries) {
            std::string::const_iterator iter;
            if (ruleEntry.character == 0 && ruleEntry.encodingIndex == 0) {
                continue;
            }

            if (ruleEntry.flag == TableRuleEntryFlag::FromFront) {
                if (ruleEntry.character > valueLen) {
                    success = false;
                    break;
                }

                iter = value.begin() + fcitx::utf8::nthChar(value, ruleEntry.character - 1);
            } else {
                if (ruleEntry.character > valueLen) {
                    success = false;
                    break;
                }

                iter = value.begin() + fcitx::utf8::nthChar(value, valueLen - ruleEntry.character);
            }
            auto prev = iter;
            iter = value.begin() + fcitx::utf8::nthChar(value, iter - value.begin(), 1);
            std::string s(prev, iter);
            s += keyValueSeparator;

            size_t len = 0;
            d->singleCharConstTrie.foreach (
                s,
                [this, d, &len, &entry](int32_t, size_t _len, DATrie<int32_t>::position_type pos) {
                    len = _len;
                    d->singleCharConstTrie.suffix(entry, _len, pos);
                    return false;
                });

            if (!len) {
                success = false;
                break;
            }

            auto key = entry;
            if (key.size() < ruleEntry.encodingIndex) {
                continue;
            }

            newKey.append(1, key[ruleEntry.encodingIndex - 1]);
        }

        if (success && !newKey.empty()) {
            key = newKey;
            return true;
        }
    }

    return false;
}

void TableBasedDictionary::statistic() {
    FCITX_D();
    std::cout << "Phrase Trie: " << d->phraseTrie.mem_size() << std::endl
              << "Single Char Trie: " << d->singleCharTrie.mem_size() << std::endl
              << "Single char const trie: " << d->singleCharConstTrie.mem_size() << std::endl
              << "Prompt Trie: " << d->promptTrie.mem_size() << std::endl
              << "pinyin phrase trie: " << d->pinyinPhraseTrie.mem_size() << std::endl;
}

void swap(TableBasedDictionary &lhs, TableBasedDictionary &rhs) noexcept {
    using std::swap;
    swap(lhs.d_ptr, rhs.d_ptr);
}
}
