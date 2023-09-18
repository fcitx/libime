/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _LIBIME_LIBIME_TABLE_TABLEBASEDDICTIONARY_P_H_
#define _LIBIME_LIBIME_TABLE_TABLEBASEDDICTIONARY_P_H_

#include "autophrasedict.h"
#include "constants.h"
#include "tablebaseddictionary.h"
#include "tableoptions.h"
#include <cstdint>
#include <optional>
#include <regex>
#include <set>
#include <vector>

namespace libime {
class TableBasedDictionaryPrivate
    : public fcitx::QPtrHolder<TableBasedDictionary> {
public:
    std::vector<TableRule> rules_;
    std::set<uint32_t> inputCode_;
    std::set<uint32_t> ignoreChars_;
    uint32_t pinyinKey_ = 0;
    uint32_t promptKey_ = 0;
    uint32_t phraseKey_ = 0;
    uint32_t codeLength_ = 0;
    DATrie<uint32_t> phraseTrie_;   // base dictionary
    DATrie<uint32_t> userTrie_;     // user dictionary
    DATrie<uint32_t> deletionTrie_; // mask over base dictionary
    uint32_t phraseTrieIndex_ = 0;
    uint32_t userTrieIndex_ = 0;
    DATrie<int32_t> singleCharTrie_; // reverse lookup from single character
    DATrie<int32_t> singleCharConstTrie_; // lookup char for new phrase
    DATrie<int32_t> singleCharLookupTrie_;
    DATrie<uint32_t> promptTrie_; // lookup for prompt;
    AutoPhraseDict autoPhraseDict_{TABLE_AUTOPHRASE_SIZE};
    TableOptions options_;
    std::optional<std::regex> autoSelectRegex_;
    std::optional<std::regex> noMatchAutoSelectRegex_;

    TableBasedDictionaryPrivate(TableBasedDictionary *q) : QPtrHolder(q) {}

    FCITX_DEFINE_SIGNAL_PRIVATE(TableBasedDictionary, tableOptionsChanged);

    std::pair<DATrie<uint32_t> *, uint32_t *> trieByFlag(PhraseFlag flag);

    std::pair<const DATrie<uint32_t> *, const uint32_t *>
    trieByFlag(PhraseFlag flag) const;

    bool insert(std::string_view key, std::string_view value, PhraseFlag flag);

    bool matchTrie(std::string_view code, TableMatchMode mode, PhraseFlag flag,
                   const TableMatchCallback &callback) const;

    void reset();
    bool validate() const;

    void loadBinary(std::istream &in);
    void loadUserBinary(std::istream &in, uint32_t version);

    FCITX_NODISCARD
    std::optional<std::tuple<std::string, std::string, PhraseFlag>>
    parseDataLine(std::string_view buf, bool user);
    void insertDataLine(std::string_view buf, bool user);
    bool matchWordsInternal(std::string_view code, TableMatchMode mode,
                            bool onlyChecking,
                            const TableMatchCallback &callback) const;

    bool validateHints(std::vector<std::string> &hints,
                       const TableRule &rule) const;
};

} // namespace libime

#endif // _LIBIME_LIBIME_TABLE_TABLEBASEDDICTIONARY_P_H_
