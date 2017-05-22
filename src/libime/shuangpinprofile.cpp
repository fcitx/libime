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
#include "shuangpinprofile.h"
#include "pinyinencoder.h"
#include "shuangpindata.h"
#include <boost/algorithm/string.hpp>
#include <boost/utility/string_view.hpp>
#include <exception>
#include <fcitx-utils/charutils.h>

namespace libime {

class ShuangpinProfilePrivate {
public:
    const SP_C *c_ = nullptr;
    const SP_S *s_ = nullptr;
    char zeroS = 'o';
};

ShuangpinProfile::ShuangpinProfile(ShuangpinBultiInProfile profile)
    : d_ptr(std::make_unique<ShuangpinProfilePrivate>()) {
    FCITX_D();
    switch (profile) {
    case ShuangpinBultiInProfile::Ziranma:
        d->c_ = SPMap_C_Ziranma;
        d->s_ = SPMap_S_Ziranma;
        break;
    case ShuangpinBultiInProfile::MS:
        d->c_ = SPMap_C_MS;
        d->s_ = SPMap_S_MS;
        break;
    case ShuangpinBultiInProfile::Ziguang:
        d->c_ = SPMap_C_Ziguang;
        d->s_ = SPMap_S_Ziguang;
        break;
    case ShuangpinBultiInProfile::ABC:
        d->c_ = SPMap_C_ABC;
        d->s_ = SPMap_S_ABC;
        break;
    case ShuangpinBultiInProfile::Zhongwenzhixing:
        d->c_ = SPMap_C_Zhongwenzhixing;
        d->s_ = SPMap_S_Zhongwenzhixing;
        break;
    case ShuangpinBultiInProfile::PinyinJiajia:
        d->c_ = SPMap_C_PinyinJiaJia;
        d->s_ = SPMap_S_PinyinJiaJia;
        break;
    case ShuangpinBultiInProfile::Xiaohe:
        d->zeroS = '*';
        d->c_ = SPMap_C_XIAOHE;
        d->s_ = SPMap_S_XIAOHE;
        break;
    default:
        throw std::invalid_argument("Invalid profile");
    }
}

ShuangpinProfile::ShuangpinProfile(std::istream &in)
    : d_ptr(std::unique_ptr<ShuangpinProfilePrivate>()) {
    FCITX_D();
    std::string line;
    auto isSpaceCheck = boost::is_any_of(" \n\t\r\v\f");
    bool isDefault = false;
    while (std::getline(in, line)) {
        boost::trim_if(line, isSpaceCheck);
        if (!line.size() || line[0] == '#') {
            continue;
        }
        boost::string_view lineView(line);

        boost::string_view option("方案名称=");
        if (boost::starts_with(lineView, option)) {
            auto name = lineView.substr(option.size()).to_string();
            boost::trim_if(name, isSpaceCheck);
            if (name == "自然码" || name == "微软" || name == "紫光" ||
                name == "拼音加加" || name == "中文之星" || name == "智能ABC" ||
                name == "小鹤") {
                isDefault = true;
            } else {
                isDefault = false;
            }
        }

        if (isDefault) {
            continue;
        }

        if (lineView[0] == '=') {
            d->zeroS = fcitx::charutils::tolower(lineView[1]);
        }

        auto equal = lineView.find('=');
        // no '=', or equal at first char, or len(substr after equal) != 1
        if (equal == boost::string_view::npos || equal == 0 ||
            equal + 2 != line.size()) {
            continue;
        }
        auto pinyin = lineView.substr(0, equal - 1);
        auto key = fcitx::charutils::tolower(lineView[equal + 1]);
        auto final = PinyinEncoder::stringToFinal(pinyin.to_string());
        if (final == PinyinFinal::Invalid) {
            auto initial = PinyinEncoder::stringToInitial(pinyin.to_string());
            if (initial == PinyinInitial::Invalid) {
                continue;
            }
        }
    }
}
}
