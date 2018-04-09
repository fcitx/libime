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
    uint32_t noSortInputLength_ = 0;
    uint32_t pinyinKey_ = '\0';
    bool autoSelect_ = false;
    int autoSelectLength_ = 0;
    int noMatchAutoSelectLength_ = 0;
    bool commitRawInput_ = false;
    std::set<uint32_t> endKey_;
    uint32_t matchingKey_ = false;
    bool exactMatch_ = false;

    bool learning_ = true;
    int autoPhraseLength_ = -1;
    int saveAutoPhraseAfter_ = -1;
    std::unordered_set<std::string> autoRuleSet_;

    // show hint for word.
    bool prompt_ = false;
    // use prompt table
    bool displayCustomPromptSymbol_ = false;

    std::string languageCode_;
};

TableOptions::TableOptions() : d_ptr(std::make_unique<TableOptionsPrivate>()) {}

FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(TableOptions)

FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, OrderPolicy, orderPolicy,
                              setOrderPolicy);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, uint32_t, noSortInputLength,
                              setNoSortInputLength);
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
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, bool, learning, setLearning);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, int, autoPhraseLength,
                              setAutoPhraseLength);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, int, saveAutoPhraseAfter,
                              setSaveAutoPhraseAfter);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, std::string, languageCode,
                              setLanguageCode);
FCITX_DEFINE_PROPERTY_PRIVATE(TableOptions, std::unordered_set<std::string>,
                              autoRuleSet, setAutoRuleSet);
} // namespace libime
