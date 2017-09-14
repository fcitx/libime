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
#include "tableoptions.h"

namespace libime {

// mostly from table.desc, certain options are not related to libime
class TableOptionsPrivate {
public:
    OrderPolicy orderPolicy_ = OrderPolicy::No;
    int noSortInputLength_ = 0;
    uint32_t pinyinKey_ = '\0';
    bool autoSelect_ = false;
    int autoSelectLength_ = 0;
    int noMatchAutoSelectLength_ = 0;
    bool commitRawInput_ = false;
    std::set<uint32_t> endKey_;
    uint32_t matchingKey_ = false;
    bool exactMatch_ = false;

    bool autoLearning_ = false;
    bool noMatchDontCommit_ = false;
    bool autoPhraseLength_ = false;
    bool saveAutoPhrase_ = false;

    // show hint for word.
    bool prompt_ = false;
    // use prompt table
    bool displayCustomPromptSymbol_ = false;
    //
    bool firstCandidateAsPreedit_ = true;
    std::unordered_set<std::string> autoRuleSet_;

    std::string languageCode_;
};

TableOptions::TableOptions() : d_ptr(std::make_unique<TableOptionsPrivate>()) {}

FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(TableOptions)

FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, OrderPolicy, orderPolicy,
                              setOrderPolicy);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, int, noSortInputLength,
                              setNoSortInputLength);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, uint32_t, pinyinKey, setPinyinKey);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, autoSelect, setAutoSelect);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, int, autoSelectLength,
                              setAutoSelectLength);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, int, noMatchAutoSelectLength,
                              setNoMatchAutoSelectLength);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, commitRawInput,
                              setCommitRawInput);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, std::set<uint32_t>, endKey,
                              setEndKey);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, uint32_t, matchingKey,
                              setMatchingKey);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, exactMatch, setExactMatch);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, autoLearning,
                              setAutoLearning);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, noMatchDontCommit,
                              setNoMatchDontCommit);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, autoPhraseLength,
                              setAutoPhraseLength);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, saveAutoPhrase,
                              setSaveAutoPhrase);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, prompt, setPrompt);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, displayCustomPromptSymbol,
                              setDisplayCustomPromptSymbol);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, firstCandidateAsPreedit,
                              setFirstCandidateAsPreedit);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, std::string, languageCode,
                              setLanguageCode);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, std::unordered_set<std::string>,
                              autoRuleSet, setAutoRuleSet);
}
