/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_TABLE_LOG_H_
#define _FCITX_LIBIME_TABLE_LOG_H_

#include <fcitx-utils/log.h>

namespace libime {
FCITX_DECLARE_LOG_CATEGORY(libime_table_logcategory);

#define LIBIME_TABLE_DEBUG()                                                   \
    FCITX_LOGC(::libime::libime_table_logcategory, Debug)
} // namespace libime

#endif // _FCITX_LIBIME_TABLE_LOG_H_
