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
#ifndef _FCITX_LIBIME_TABLE_TABLEIME_H_
#define _FCITX_LIBIME_TABLE_TABLEIME_H_

#include "libimetable_export.h"
#include <boost/utility/string_view.hpp>
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/macros.h>
#include <libime/core/decoder.h>
#include <libime/core/userlanguagemodel.h>
#include <libime/table/tableoptions.h>
#include <memory>

namespace libime {

class TableIMEPrivate;
class TableBasedDictionary;

class LIBIMETABLE_EXPORT TableIME : public fcitx::ConnectableObject {
public:
    TableIME(LanguageModelResolver *lmProvider);
    virtual ~TableIME();

    TableBasedDictionary *requestDict(boost::string_view dictName);
    void saveDict(TableBasedDictionary *dict);
    UserLanguageModel *languageModelForDictionary(TableBasedDictionary *dict);

protected:
    virtual TableBasedDictionary *
    requestDictImpl(boost::string_view dictName) = 0;
    virtual void saveDictImpl(TableBasedDictionary *dict) = 0;

private:
    std::unique_ptr<TableIMEPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TableIME);
};
}

#endif // _FCITX_LIBIME_TABLE_TABLEIME_H_
