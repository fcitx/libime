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
#ifndef _FCITX_LIBIME_UTF8_PINYINDICTIONARY_H_
#define _FCITX_LIBIME_UTF8_PINYINDICTIONARY_H_

#include "libime_export.h"
#include <fcitx-utils/macros.h>
#include <memory>

namespace libime {

enum class PinyinDictFormat { Text, Binary };

class PinyinDictionaryPrivate;

typedef std::function<bool(const char *encodedPinyin, const std::string &hanzi, float cost)> MatchCallback;

class LIBIME_EXPORT PinyinDictionary {
public:
    explicit PinyinDictionary(const char *filename, PinyinDictFormat format);
    ~PinyinDictionary();

    void matchWords(const char *data, size_t size, MatchCallback callback);
    void matchWords(const char *initials, const char *finals, size_t size, MatchCallback callback);

    void save(const char *filename);
    void save(std::ostream &out);
    void dump(std::ostream &out);

private:
    void build(std::ifstream &in);
    void open(std::ifstream &in);

    std::unique_ptr<PinyinDictionaryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(PinyinDictionary);
};
}

#endif // _FCITX_LIBIME_UTF8_PINYINDICTIONARY_H_
