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
#ifndef _FCITX_LIBIME_LANGUAGEMODEL_H_
#define _FCITX_LIBIME_LANGUAGEMODEL_H_

#include "libime_export.h"
#include <boost/utility/string_view.hpp>
#include <fcitx-utils/macros.h>
#include <memory>
#include <vector>

namespace libime {

using WordIndex = unsigned int;
using State = std::vector<char>;

class LanguageModelPrivate;
class LIBIME_EXPORT LanguageModel {
public:
    LanguageModel(const char *file);
    virtual ~LanguageModel();

    WordIndex beginSentence() const;
    WordIndex endSentence() const;
    WordIndex unknown() const;
    State beginState() const;
    State nullState() const;
    WordIndex index(boost::string_view view) const;
    float score(const State &state, WordIndex word, State &out) const;

private:
    std::unique_ptr<LanguageModelPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(LanguageModel);
};
}

#endif // _FCITX_LIBIME_LANGUAGEMODEL_H_
