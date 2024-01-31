/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_PINYIN_SHUANGPINPROFILE_H_
#define _FCITX_LIBIME_PINYIN_SHUANGPINPROFILE_H_

#include "libimepinyin_export.h"
#include <fcitx-utils/macros.h>
#include <istream>
#include <libime/pinyin/pinyinencoder.h>
#include <map>
#include <memory>
#include <set>
#include <string>

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

class LIBIMEPINYIN_EXPORT ShuangpinProfile {
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
    const ValidInputSetType &validInitial() const;

private:
    void buildShuangpinTable();
    std::unique_ptr<ShuangpinProfilePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ShuangpinProfile);
};
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_SHUANGPINPROFILE_H_
