/*
 * SPDX-FileCopyrightText: 2015-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef LIBIME_UTILS_H
#define LIBIME_UTILS_H

#include <fcitx-utils/log.h>
#include <libime/core/libimecore_export.h>

namespace libime {

LIBIMECORE_EXPORT FCITX_DECLARE_LOG_CATEGORY(libime_logcategory);
#define LIBIME_DEBUG() FCITX_LOGC(::libime::libime_logcategory, Debug)
#define LIBIME_ERROR() FCITX_LOGC(::libime::libime_logcategory, Error)
} // namespace libime

#endif // LIBIME_UTILS_H
