/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "inputbuffer.h"
#include <cstddef>
#include <fcitx-utils/utf8.h>
#include <string_view>

namespace libime {

std::string_view InputBuffer::at(size_t i) const { return viewAt(i); }
} // namespace libime
