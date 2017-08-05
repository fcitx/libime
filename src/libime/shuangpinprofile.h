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
#ifndef _FCITX_LIBIME_SHUANGPINPROFILE_H_
#define _FCITX_LIBIME_SHUANGPINPROFILE_H_

#include <fcitx-utils/macros.h>

#include "libime_export.h"
#include "pinyinencoder.h"
#include <istream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

namespace libime {

enum class ShuangpinBuiltinProfile {
    Ziranma,
    MS,
    Ziguang,
    ABC,
    Zhongwenzhixing,
    PinyinJiajia,
    Xiaohe,
};

class ShuangpinProfilePrivate;

class LIBIME_EXPORT ShuangpinProfile {
public:
    typedef std::map<std::string,
                     std::multimap<PinyinSyllable, PinyinFuzzyFlags>>
        TableType;
    typedef std::set<char> ValidInputSetType;
    explicit ShuangpinProfile(ShuangpinBuiltinProfile profile);
    explicit ShuangpinProfile(std::istream &in);

    FCITX_DECLARE_VIRTUAL_DTOR_COPY_AND_MOVE(ShuangpinProfile)

    const TableType &table() const;
    const ValidInputSetType &validInput() const;

private:
    void buildShuangpinTable();
    std::unique_ptr<ShuangpinProfilePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ShuangpinProfile);
};
}

#endif // _FCITX_LIBIME_SHUANGPINPROFILE_H_
