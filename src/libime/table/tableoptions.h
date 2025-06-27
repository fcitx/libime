/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_TABLE_TABLEOPTIONS_H_
#define _FCITX_LIBIME_TABLE_TABLEOPTIONS_H_

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <fcitx-utils/macros.h>
#include <libime/table/libimetable_export.h>

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
    FCITX_DECLARE_PROPERTY(uint32_t, noSortInputLength, setNoSortInputLength);
    FCITX_DECLARE_PROPERTY(bool, autoSelect, setAutoSelect);
    FCITX_DECLARE_PROPERTY(int, autoSelectLength, setAutoSelectLength);
    FCITX_DECLARE_PROPERTY(std::string, autoSelectRegex, setAutoSelectRegex);
    FCITX_DECLARE_PROPERTY(int, noMatchAutoSelectLength,
                           setNoMatchAutoSelectLength);
    FCITX_DECLARE_PROPERTY(std::string, noMatchAutoSelectRegex,
                           setNoMatchAutoSelectRegex);
    FCITX_DECLARE_PROPERTY(bool, commitRawInput, setCommitRawInput);
    FCITX_DECLARE_PROPERTY(std::set<uint32_t>, endKey, setEndKey);
    FCITX_DECLARE_PROPERTY(uint32_t, matchingKey, setMatchingKey);
    FCITX_DECLARE_PROPERTY(bool, exactMatch, setExactMatch);
    FCITX_DECLARE_PROPERTY(bool, learning, setLearning);
    FCITX_DECLARE_PROPERTY(int, autoPhraseLength, setAutoPhraseLength);
    FCITX_DECLARE_PROPERTY(int, saveAutoPhraseAfter, setSaveAutoPhraseAfter);
    FCITX_DECLARE_PROPERTY(std::unordered_set<std::string>, autoRuleSet,
                           setAutoRuleSet);
    FCITX_DECLARE_PROPERTY(std::string, languageCode, setLanguageCode);
    FCITX_DECLARE_PROPERTY(bool, sortByCodeLength, setSortByCodeLength);

private:
    std::unique_ptr<TableOptionsPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TableOptions);
};
} // namespace libime

#endif // _FCITX_LIBIME_TABLE_TABLEOPTIONS_H_
