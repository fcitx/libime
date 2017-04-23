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
#ifndef _FCITX_LIBIME_USERLANGUAGEMODEL_H_
#define _FCITX_LIBIME_USERLANGUAGEMODEL_H_

#include "languagemodel.h"
#include "libime_export.h"

namespace libime {

class UserLanguageModelPrivate;

class LIBIME_EXPORT UserLanguageModel : public LanguageModel {
public:
    UserLanguageModel(const char *sysfile, const char *userFile);
    virtual ~UserLanguageModel();

    const State &beginState() const override;
    const State &nullState() const override;
    float score(const State &state, const WordNode *word,
                State &out) const override;

private:
    std::unique_ptr<UserLanguageModelPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(UserLanguageModel);
};
}

#endif // _FCITX_LIBIME_USERLANGUAGEMODEL_H_
