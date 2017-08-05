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
#ifndef _FCITX_LIBIME_TABLEIME_H_
#define _FCITX_LIBIME_TABLEIME_H_

#include "decoder.h"
#include "libime_export.h"
#include "tableoptions.h"
#include "userlanguagemodel.h"
#include <boost/utility/string_view.hpp>
#include <fcitx-utils/dynamictrackableobject.h>
#include <fcitx-utils/macros.h>
#include <memory>

namespace libime {

class TableIMEPrivate;
class TableBasedDictionary;

class TableDictionrayResolver {
public:
    virtual ~TableDictionrayResolver() = default;
    virtual TableBasedDictionary *requestDict(boost::string_view name) = 0;
    virtual void saveDict(TableBasedDictionary *dict) = 0;
};

class LIBIME_EXPORT TableIME : public fcitx::DynamicTrackableObject {
public:
    TableIME(std::unique_ptr<TableDictionrayResolver> dictProvider,
             std::unique_ptr<LanguageModelResolver> lmProvider);
    virtual ~TableIME();

    TableBasedDictionary *requestDict(boost::string_view dictName);
    void saveDict(TableBasedDictionary *dict);
    void clear();
    UserLanguageModel *languageModelForDictionary(TableBasedDictionary *dict);

private:
    std::unique_ptr<TableIMEPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TableIME);
};
}

#endif // _FCITX_LIBIME_TABLEIME_H_
