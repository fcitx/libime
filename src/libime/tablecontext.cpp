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
#include "tablecontext.h"
#include "tablebaseddictionary.h"
#include "userlanguagemodel.h"

namespace libime {

class TableContextPrivate {
public:
    std::shared_ptr<TableBasedDictionary> dict_;
    std::shared_ptr<UserLanguageModel> model_;
};

TableContext::TableContext(TableIME *ime) : InputBuffer(true) {}

TableContext::~TableContext() { destroy(); }

bool TableContext::cancelTill(size_t pos) { return false; }

void TableContext::typeImpl(const char *s, size_t length) {
    cancelTill(cursor());
    InputBuffer::typeImpl(s, length);
    update();
}

void TableContext::setCursor(size_t pos) {
    auto cancelled = cancelTill(pos);
    InputBuffer::setCursor(pos);
    if (cancelled) {
        update();
    }
}

void TableContext::erase(size_t from, size_t to) {
    if (from == to) {
        return;
    }

    // check if erase everything
    if (from == 0 && to >= size()) {
        FCITX_D();
        // FIXME
    } else {
        cancelTill(from);
    }
    InputBuffer::erase(from, to);

    if (size()) {
        update();
    }
}

void TableContext::update() {}
}
