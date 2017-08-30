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
#include "pinyindata.h"
#include "pinyinencoder.h"
#include "shuangpindata.h"
#include <boost/algorithm/string.hpp>
#include <boost/utility/string_view.hpp>
#include <exception>
#include <fcitx-utils/charutils.h>
#include <iostream>
#include <set>

namespace libime {

class ShuangpinProfilePrivate {
public:
    ShuangpinProfilePrivate() = default;
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(ShuangpinProfilePrivate)

    char zeroS_ = 'o';
    std::unordered_multimap<char, PinyinFinal> finalMap_;
    std::unordered_multimap<char, PinyinInitial> initialMap_;
    std::set<PinyinFinal> finalSet_;
    ShuangpinProfile::ValidInputSetType validInputs_;
    ShuangpinProfile::TableType spTable_;
};

ShuangpinProfile::ShuangpinProfile(ShuangpinBuiltinProfile profile)
    : d_ptr(std::make_unique<ShuangpinProfilePrivate>()) {
    FCITX_D();
    const SP_C *c = nullptr;
    const SP_S *s = nullptr;
    switch (profile) {
    case ShuangpinBuiltinProfile::Ziranma:
        c = SPMap_C_Ziranma;
        s = SPMap_S_Ziranma;
        break;
    case ShuangpinBuiltinProfile::MS:
        c = SPMap_C_MS;
        s = SPMap_S_MS;
        break;
    case ShuangpinBuiltinProfile::Ziguang:
        c = SPMap_C_Ziguang;
        s = SPMap_S_Ziguang;
        break;
    case ShuangpinBuiltinProfile::ABC:
        c = SPMap_C_ABC;
        s = SPMap_S_ABC;
        break;
    case ShuangpinBuiltinProfile::Zhongwenzhixing:
        c = SPMap_C_Zhongwenzhixing;
        s = SPMap_S_Zhongwenzhixing;
        break;
    case ShuangpinBuiltinProfile::PinyinJiajia:
        c = SPMap_C_PinyinJiaJia;
        s = SPMap_S_PinyinJiaJia;
        break;
    case ShuangpinBuiltinProfile::Xiaohe:
        d->zeroS_ = '*';
        c = SPMap_C_XIAOHE;
        s = SPMap_S_XIAOHE;
        break;
    default:
        throw std::invalid_argument("Invalid profile");
    }

    for (auto i = 0; c[i].cJP; i++) {
        auto final = PinyinEncoder::stringToFinal(c[i].strQP);
        d->finalMap_.emplace(c[i].cJP, final);
        d->finalSet_.insert(final);
    }

    for (auto i = 0; s[i].cJP; i++) {
        d->initialMap_.emplace(s[i].cJP,
                               PinyinEncoder::stringToInitial(s[i].strQP));
    }

    buildShuangpinTable();
}

ShuangpinProfile::ShuangpinProfile(std::istream &in)
    : d_ptr(std::make_unique<ShuangpinProfilePrivate>()) {
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
            d->zeroS_ = fcitx::charutils::tolower(lineView[1]);
        }

        auto equal = lineView.find('=');
        // no '=', or equal at first char, or len(substr after equal) != 1
        if (equal == boost::string_view::npos || equal == 0 ||
            equal + 2 != line.size()) {
            continue;
        }
        auto pinyin = lineView.substr(0, equal);
        auto key = fcitx::charutils::tolower(lineView[equal + 1]);
        auto final = PinyinEncoder::stringToFinal(pinyin.to_string());
        if (final == PinyinFinal::Invalid) {
            auto initial = PinyinEncoder::stringToInitial(pinyin.to_string());
            if (initial == PinyinInitial::Invalid) {
                continue;
            }
            d->initialMap_.emplace(key, initial);
        } else {
            d->finalMap_.emplace(key, final);
        }
    }

    buildShuangpinTable();
}

FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(ShuangpinProfile)

void ShuangpinProfile::buildShuangpinTable() {
    FCITX_D();
    for (char c = 'a'; c <= 'z'; c++) {
        d->validInputs_.insert(c);
    }
    for (auto &p : d->initialMap_) {
        d->validInputs_.insert(p.first);
    }
    for (auto &p : d->finalMap_) {
        d->validInputs_.insert(p.first);
    }

    std::set<char> initialChars;
    if (d->zeroS_ != '*') {
        d->validInputs_.insert(d->zeroS_);
        initialChars.insert(d->zeroS_);
    }

    for (auto c = PinyinEncoder::firstInitial; c <= PinyinEncoder::lastInitial;
         c++) {
        const auto &initialString =
            PinyinEncoder::initialToString(static_cast<PinyinInitial>(c));
        if (initialString.size() == 1) {
            initialChars.insert(initialString[0]);
        }
    }

    for (auto &p : d->initialMap_) {
        initialChars.insert(p.first);
    }

    std::set<char> finalChars;
    for (auto c = PinyinEncoder::firstFinal; c <= PinyinEncoder::lastFinal;
         c++) {
        const auto &finalString =
            PinyinEncoder::finalToString(static_cast<PinyinFinal>(c));
        if (finalString.size() == 1) {
            finalChars.insert(finalString[0]);
        }
    }

    for (auto &p : d->finalMap_) {
        finalChars.insert(p.first);
    }

    auto addPinyinToList = [](
        std::multimap<PinyinSyllable, PinyinFuzzyFlags> &pys, PinyinInitial i,
        PinyinFinal f, PinyinFuzzyFlags flags) {
        PinyinSyllable s(i, f);
        if (flags == PinyinFuzzyFlag::None) {

            auto iter = pys.find(s);
            if (iter != pys.end() && iter->second != PinyinFuzzyFlag::None) {
                pys.erase(s);
            }
            if (iter == pys.end()) {
                pys.emplace(s, flags);
            }
        } else {
            auto iterPair = pys.equal_range(s);
            // no match
            if (iterPair.first != iterPair.second) {
                if (iterPair.first->second == PinyinFuzzyFlag::None) {
                    return;
                }
                // check dup
                for (auto i = iterPair.first; i != iterPair.second; i++) {
                    if (i->second == flags) {
                        return;
                    }
                }
            }

            pys.emplace(s, flags);
        }
    };

    auto addPinyin = [addPinyinToList,
                      d](std::multimap<PinyinSyllable, PinyinFuzzyFlags> &pys,
                         const std::string &py) {
        auto &map = getPinyinMap();
        auto iterPair = map.equal_range(py);
        if (iterPair.first != iterPair.second) {
            for (auto &item :
                 boost::make_iterator_range(iterPair.first, iterPair.second)) {
                addPinyinToList(pys, item.initial(), item.final(),
                                item.flags());
            }
        }
    };

    if (d->zeroS_ == '*') {
        for (auto c : finalChars) {
            std::string input{c, c};
            auto &pys = d->spTable_[input];
            auto final = PinyinEncoder::stringToFinal(std::string{c});
            if (final != PinyinFinal::Invalid &&
                PinyinEncoder::isValidInitialFinal(PinyinInitial::Zero,
                                                   final)) {
                pys.emplace(PinyinSyllable{PinyinInitial::Zero, final},
                            PinyinFuzzyFlag::None);
            }
            if (d->finalMap_.count(c)) {
                auto finalIterPair = d->finalMap_.equal_range(c);
                if (finalIterPair.first != finalIterPair.second) {
                    for (auto &item : boost::make_iterator_range(
                             finalIterPair.first, finalIterPair.second)) {
                        if (PinyinEncoder::isValidInitialFinal(
                                PinyinInitial::Zero, item.second) &&
                            PinyinEncoder::finalToString(item.second).size() !=
                                2) {
                            pys.emplace(PinyinSyllable{PinyinInitial::Zero,
                                                       item.second},
                                        PinyinFuzzyFlag::None);
                        }
                    }
                }
            }
            if (!pys.size()) {
                d->spTable_.erase(input);
            }
        }
    }

    for (auto c1 : initialChars) {
        for (auto c2 : finalChars) {
            std::string input{c1, c2};
            auto &pys = d->spTable_[input];

            if (input == "oo") {
                input = input;
            }

            std::vector<PinyinInitial> initials;
            std::vector<PinyinFinal> finals;
            auto initialIterPair = d->initialMap_.equal_range(c1);
            if (initialIterPair.first != initialIterPair.second) {
                for (auto &item : boost::make_iterator_range(
                         initialIterPair.first, initialIterPair.second)) {
                    initials.push_back(item.second);
                }
            }
            auto initial = PinyinEncoder::stringToInitial(std::string{c1});
            if (initial != PinyinInitial::Invalid) {
                initials.push_back(initial);
            }

            if (c1 == d->zeroS_) {
                initials.push_back(PinyinInitial::Zero);
            }

            auto finalIterPair = d->finalMap_.equal_range(c2);
            if (finalIterPair.first != finalIterPair.second) {
                for (auto &item : boost::make_iterator_range(
                         finalIterPair.first, finalIterPair.second)) {
                    finals.push_back(item.second);
                }
            }
            auto final = PinyinEncoder::stringToFinal(std::string{c2});
            if (!d->finalSet_.count(final)) {
                // v is defined in map.
                if (final != PinyinFinal::Invalid) {
                    finals.push_back(final);
                }
            }

            for (auto i : initials) {
                for (auto f : finals) {
                    auto py = PinyinEncoder::initialToString(i) +
                              PinyinEncoder::finalToString(f);
                    addPinyin(pys, py);
                }
            }

            if (!pys.size()) {
                d->spTable_.erase(input);
            }
        }
    }

    for (auto &p : getPinyinMap()) {
        if (p.pinyin().size() == 2 && p.initial() == PinyinInitial::Zero &&
            (!d->spTable_.count(p.pinyin().to_string()) || d->zeroS_ == '*')) {
            auto &pys = d->spTable_[p.pinyin().to_string()];
            pys.emplace(PinyinSyllable{p.initial(), p.final()}, p.flags());
        }
    }

    for (char c : d->validInputs_) {
        std::string input{c};
        auto &pys = d->spTable_[input];
        auto initial = PinyinEncoder::stringToInitial(std::string{c});
        if (initial != PinyinInitial::Invalid) {
            addPinyinToList(pys, initial, PinyinFinal::Invalid,
                            PinyinFuzzyFlag::None);
        }
        auto initialIterPair = d->initialMap_.equal_range(c);
        for (auto &item : boost::make_iterator_range(initialIterPair.first,
                                                     initialIterPair.second)) {
            addPinyinToList(pys, item.second, PinyinFinal::Invalid,
                            PinyinFuzzyFlag::None);
        }
        auto final = PinyinEncoder::stringToFinal(std::string{c});
        if (final != PinyinFinal::Invalid &&
            PinyinEncoder::isValidInitialFinal(PinyinInitial::Zero, final) &&
            pys.empty()) {
            addPinyinToList(pys, PinyinInitial::Zero, final,
                            PinyinFuzzyFlag::None);
        }

        // do not look into finalMap_
        // because we're doing sp, using sp with single final shouldn't be
        // allowed
        if (pys.empty()) {
            d->spTable_.erase(input);
        }
    }
}

const ShuangpinProfile::TableType &ShuangpinProfile::table() const {
    FCITX_D();
    return d->spTable_;
}

const ShuangpinProfile::ValidInputSetType &
ShuangpinProfile::validInput() const {
    FCITX_D();
    return d->validInputs_;
}
}
