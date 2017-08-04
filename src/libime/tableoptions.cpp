/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#include "tableoptions.h"

namespace libime {

// mostly from table.desc, certain options are not related to libime
class TableOptionsPrivate {
public:
    OrderPolicy orderPolicy_ = OrderPolicy::No;
    int noSortInputLength_ = 0;
    bool usePinyin_ = false;
    char pinyinKey_ = '\0';
    bool autoCommit_ = false;
    int autoCommitLength_ = 0;
    int noMatchAutoCommitLength_ = 0;
    bool commitRawInput_ = false;
    std::string endKey_;
    char matchingKey_ = false;
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

    std::string languageCode_;
};

TableOptions::TableOptions() : d_ptr(std::make_unique<TableOptionsPrivate>()) {}
TableOptions::TableOptions(const libime::TableOptions &options)
    : d_ptr(std::make_unique<TableOptionsPrivate>(*options.d_ptr)) {}

TableOptions::TableOptions(libime::TableOptions &&options)
    : d_ptr(std::move(options.d_ptr)) {}

TableOptions::~TableOptions() {}

TableOptions &TableOptions::operator=(TableOptions other) {
    using std::swap;
    swap(d_ptr, other.d_ptr);
    return *this;
}

FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, OrderPolicy, orderPolicy,
                              setOrderPolicy);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, int, noSortInputLength,
                              setNoSortInputLength);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, char, pinyinKey, setPinyinKey);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, autoCommit, setAutoCommit);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, int, autoCommitLength,
                              setAutoCommitLength);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, int, noMatchAutoCommitLength,
                              setNoMatchAutoCommitLength);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, commitRawInput,
                              setCommitRawInput);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, std::string, endKey, setEndKey);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, char, matchingKey, setMatchingKey);
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
}
