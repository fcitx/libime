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
#ifndef _FCITX_LIBIME_TABLECONTEXT_H_
#define _FCITX_LIBIME_TABLECONTEXT_H_

#include "inputbuffer.h"
#include "lattice.h"
#include "libime_export.h"
#include <boost/utility/string_view.hpp>
#include <fcitx-utils/dynamictrackableobject.h>
#include <fcitx-utils/macros.h>

namespace libime {

class TableContextPrivate;
class TableBasedDictionary;
class UserLanguageModel;

class LIBIME_EXPORT TableContext : public InputBuffer {
public:
    TableContext(TableBasedDictionary &dict, UserLanguageModel &model);
    virtual ~TableContext();

    void erase(size_t from, size_t to) override;
    void setCursor(size_t pos) override;

    void select(size_t idx);
    void cancel();
    bool cancelTill(size_t pos);

    bool isValidInput(char c) const;

    const std::vector<SentenceResult> &candidates() const;
#if 0
    std::string preedit() const;
    std::pair<std::string, size_t> preeditWithCursor() const;
    std::string selectedSentence() const;
    size_t selectedLength() const;
#endif

    const TableBasedDictionary &dict() const;

protected:
    void typeImpl(const char *s, size_t length) override;

private:
    void update();
    void typeOneChar(const char *s, size_t length);

    std::unique_ptr<TableContextPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TableContext);
};
}

#endif // _FCITX_LIBIME_TABLECONTEXT_H_
