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
#ifndef _FCITX_LIBIME_TABLE_TABLEOPTIONS_H_
#define _FCITX_LIBIME_TABLE_TABLEOPTIONS_H_

#include "libimetable_export.h"
#include <cstdint>
#include <fcitx-utils/macros.h>
#include <memory>
#include <set>
#include <unordered_set>

namespace libime {

enum class OrderPolicy {
    No,
    Fast,
    Freq,
};

class TableOptionsPrivate;

class LIBIMETABLE_EXPORT TableOptions {
public:
    TableOptions();
    FCITX_DECLARE_VIRTUAL_DTOR_COPY_AND_MOVE(TableOptions)

    FCITX_DECLARE_PROPERTY(OrderPolicy, orderPolicy, setOrderPolicy);
    FCITX_DECLARE_PROPERTY(int, noSortInputLength, setNoSortInputLength);
    FCITX_DECLARE_PROPERTY(uint32_t, pinyinKey, setPinyinKey);
    FCITX_DECLARE_PROPERTY(bool, autoSelect, setAutoSelect);
    FCITX_DECLARE_PROPERTY(int, autoSelectLength, setAutoSelectLength);
    FCITX_DECLARE_PROPERTY(int, noMatchAutoSelectLength,
                           setNoMatchAutoSelectLength);
    FCITX_DECLARE_PROPERTY(bool, commitRawInput, setCommitRawInput);
    FCITX_DECLARE_PROPERTY(std::set<uint32_t>, endKey, setEndKey);
    FCITX_DECLARE_PROPERTY(uint32_t, matchingKey, setMatchingKey);
    FCITX_DECLARE_PROPERTY(bool, exactMatch, setExactMatch);
    FCITX_DECLARE_PROPERTY(bool, autoLearning, setAutoLearning);
    FCITX_DECLARE_PROPERTY(bool, noMatchDontCommit, setNoMatchDontCommit);
    FCITX_DECLARE_PROPERTY(bool, autoPhraseLength, setAutoPhraseLength);
    FCITX_DECLARE_PROPERTY(bool, saveAutoPhrase, setSaveAutoPhrase);
    FCITX_DECLARE_PROPERTY(bool, prompt, setPrompt);
    FCITX_DECLARE_PROPERTY(bool, displayCustomPromptSymbol,
                           setDisplayCustomPromptSymbol);
    FCITX_DECLARE_PROPERTY(bool, firstCandidateAsPreedit,
                           setFirstCandidateAsPreedit);
    FCITX_DECLARE_PROPERTY(std::unordered_set<std::string>, autoRuleSet,
                           setAutoRuleSet);
    FCITX_DECLARE_PROPERTY(std::string, languageCode, setLanguageCode);

private:
    std::unique_ptr<TableOptionsPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TableOptions);
};
}

#endif // _FCITX_LIBIME_TABLE_TABLEOPTIONS_H_
