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
#include "tableime.h"
#include "tablebaseddictionary.h"
#include <boost/utility/string_view.hpp>
#include <map>

namespace libime {

class TableIMEPrivate {
public:
    TableIMEPrivate(std::unique_ptr<TableDictionrayResolver> dictProvider,
                    std::unique_ptr<LanguageModelResolver> lmProvider)
        : dictProvider_(std::move(dictProvider)),
          lmProvider_(std::move(lmProvider)) {}

    std::unique_ptr<TableDictionrayResolver> dictProvider_;
    std::unique_ptr<LanguageModelResolver> lmProvider_;
    std::map<std::string, std::unique_ptr<TableBasedDictionary>, std::less<>>
        dicts_;
    std::map<std::string, std::unique_ptr<UserLanguageModel>, std::less<>>
        languageModels_;
};

TableIME::TableIME(std::unique_ptr<TableDictionrayResolver> dictProvider,
                   std::unique_ptr<LanguageModelResolver> lmProvider)
    : d_ptr(std::make_unique<TableIMEPrivate>(std::move(dictProvider),
                                              std::move(lmProvider))) {}

TableIME::~TableIME() { destroy(); }

TableBasedDictionary *TableIME::requestDict(boost::string_view dictName) {
    FCITX_D();
    auto iter = d->dicts_.find(dictName);
    if (iter == d->dicts_.end()) {
        std::unique_ptr<TableBasedDictionary> dict(
            d->dictProvider_->requestDict(dictName));
        if (!dict) {
            return nullptr;
        }
        iter = d->dicts_.emplace(dictName.to_string(), std::move(dict)).first;
    }

    return iter->second.get();
}

void TableIME::saveDict(TableBasedDictionary *dict) {
    FCITX_D();
    d->dictProvider_->saveDict(dict);
}

UserLanguageModel *
TableIME::languageModelForDictionary(TableBasedDictionary *dict) {
    FCITX_D();
    auto language = dict->tableOptions().languageCode();
    auto iter = d->languageModels_.find(language);
    if (iter == d->languageModels_.end()) {
        auto fileName = d->lmProvider_->languageModelFileForLanguage(
            dict->tableOptions().languageCode());
        if (!fileName.empty()) {
            return nullptr;
        }

        iter =
            d->languageModels_
                .emplace(language,
                         std::make_unique<UserLanguageModel>(fileName.data()))
                .first;
    }
    return iter->second.get();
}
}
