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
#ifndef _FCITX_LIBIME_PINYINCONTEXT_H_
#define _FCITX_LIBIME_PINYINCONTEXT_H_

#include "inputbuffer.h"
#include "libime_export.h"
#include <fcitx-utils/macros.h>
#include <memory>

namespace libime {
class PinyinIME;
class PinyinContextPrivate;

class LIBIME_EXPORT PinyinContext : public InputBuffer {
public:
    PinyinContext(PinyinIME *ime);
    virtual ~PinyinContext();

    void type(boost::string_view s) override;
    void erase(size_t from, size_t to) override;

private:
    std::unique_ptr<PinyinContextPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(PinyinContext);
};
}

#endif // _FCITX_LIBIME_PINYINCONTEXT_H_
