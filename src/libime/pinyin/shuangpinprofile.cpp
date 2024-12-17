/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "shuangpinprofile.h"
#include "pinyincorrectionprofile.h"
#include "pinyindata.h"
#include "pinyinencoder.h"
#include "shuangpindata.h"
#include <boost/algorithm/string.hpp>
#include <fcitx-utils/charutils.h>
#include <set>
#include <string_view>
#include <unordered_map>

namespace libime {

class ShuangpinProfilePrivate {
public:
    ShuangpinProfilePrivate() = default;
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(ShuangpinProfilePrivate)

    std::string zeroS_ = "o";
    std::unordered_multimap<char, PinyinFinal> finalMap_;
    std::unordered_multimap<char, PinyinInitial> initialMap_;
    std::unordered_multimap<std::string, std::pair<PinyinInitial, PinyinFinal>>
        initialFinalMap_;
    std::set<PinyinFinal> finalSet_;
    ShuangpinProfile::ValidInputSetType validInputs_;
    ShuangpinProfile::ValidInputSetType validInitials_;
    ShuangpinProfile::TableType spTable_;

    void buildShuangpinTable(const PinyinCorrectionProfile *correctionProfile) {
        // Set up valid inputs.
        for (char c = 'a'; c <= 'z'; c++) {
            validInputs_.insert(c);
        }
        for (const auto &p : initialMap_) {
            validInputs_.insert(p.first);
        }
        std::unordered_map<PinyinFinal, char> singleCharFinal;
        for (const auto &p : finalMap_) {
            validInputs_.insert(p.first);
            if (PinyinEncoder::finalToString(p.second).size() == 1) {
                singleCharFinal[p.second] = p.first;
            }
        }

        for (const auto &p : initialFinalMap_) {
            for (auto c : p.first) {
                validInputs_.insert(c);
            }
        }

        std::set<char> initialChars;
        for (auto zero : zeroS_) {
            if (zero != '*') {
                validInputs_.insert(zero);
                initialChars.insert(zero);
            }
        }

        // Collect all initial and final chars.
        // Add single char initial to initialChars.
        for (auto c = PinyinEncoder::firstInitial;
             c <= PinyinEncoder::lastInitial; c++) {
            const auto &initialString =
                PinyinEncoder::initialToString(static_cast<PinyinInitial>(c));
            if (initialString.size() == 1) {
                initialChars.insert(initialString[0]);
            }
        }
        // Add char in map to initialChars.
        for (auto &p : initialMap_) {
            initialChars.insert(p.first);
        }

        // Collect all final chars.
        // Add single char final to finalChars.
        std::set<char> finalChars;
        for (auto c = PinyinEncoder::firstFinal; c <= PinyinEncoder::lastFinal;
             c++) {
            auto f = static_cast<PinyinFinal>(c);
            const auto &finalString = PinyinEncoder::finalToString(f);
            if (finalString.size() == 1 && !singleCharFinal.count(f)) {
                finalChars.insert(finalString[0]);
                singleCharFinal[f] = finalString[0];
            }
        }
        // Add final in map to finalChars
        for (auto &p : finalMap_) {
            finalChars.insert(p.first);
        }

        for (const auto &[final, chr] : singleCharFinal) {
            auto [begin, end] = finalMap_.equal_range(chr);
            if (std::find_if(begin, end, [final = final](const auto &item) {
                    return item.second == final;
                }) == end) {
                finalMap_.emplace(chr, final);
            }
        }

        auto addPinyinToList =
            [](std::multimap<PinyinSyllable, PinyinFuzzyFlags> &pys,
               PinyinInitial i, PinyinFinal f, PinyinFuzzyFlags flags) {
                PinyinSyllable s(i, f);
                if (flags == PinyinFuzzyFlag::None) {

                    auto iter = pys.find(s);
                    // We replace fuzzy with non-fuzzy.
                    if (iter != pys.end() &&
                        iter->second != PinyinFuzzyFlag::None) {
                        pys.erase(s);
                        iter = pys.end();
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
                        for (auto i = iterPair.first; i != iterPair.second;
                             i++) {
                            if (i->second == flags) {
                                return;
                            }
                        }
                    }

                    pys.emplace(s, flags);
                }
            };

        auto addPinyin =
            [addPinyinToList](
                std::multimap<PinyinSyllable, PinyinFuzzyFlags> &pys,
                const std::string &py) {
                const auto &map = getPinyinMapV2();
                auto iterPair = map.equal_range(py);
                if (iterPair.first != iterPair.second) {
                    for (const auto &item : boost::make_iterator_range(
                             iterPair.first, iterPair.second)) {
                        // Shuangpin should not consider advanced typo, since
                        // it's swapping character order and will leads to wrong
                        // entry. Common typo also have "ng->gn" is ok.
                        if (item.flags().test(PinyinFuzzyFlag::AdvancedTypo)) {
                            continue;
                        }
                        addPinyinToList(pys, item.initial(), item.final(),
                                        item.flags());
                    }
                }
            };

        // Special handling for Ziranma & Xiaohe style.
        if (zeroS_.find('*') != std::string::npos) {
            // length 1: aeiou, repeat it once: e.g. aa
            // length 2: keep same as quanpin
            // length 3: use the initial of quanpin and the one in the table.
            for (auto c : finalChars) {
                // If c is in final map.
                auto finalIterPair = finalMap_.equal_range(c);
                for (auto &item : boost::make_iterator_range(
                         finalIterPair.first, finalIterPair.second)) {
                    if (PinyinEncoder::isValidInitialFinal(PinyinInitial::Zero,
                                                           item.second)) {
                        std::string input;
                        const auto &finalString =
                            PinyinEncoder::finalToString(item.second);
                        if (finalString.size() == 1) {
                            input = std::string{c, c};
                        } else {
                            auto final = PinyinEncoder::stringToFinal(
                                std::string{finalString[0]});
                            if (final != PinyinFinal::Invalid) {
                                auto singleCharFinalIter =
                                    singleCharFinal.find(final);
                                if (singleCharFinalIter !=
                                    singleCharFinal.end()) {
                                    input = std::string{
                                        singleCharFinalIter->second, c};
                                }
                            }
                        }
                        spTable_[input].emplace(
                            PinyinSyllable{PinyinInitial::Zero, item.second},
                            PinyinFuzzyFlag::None);
                    }
                }
            }
        }

        // Enumerate the combinition of initial + final
        for (auto c1 : initialChars) {
            for (auto c2 : finalChars) {
                std::string input{c1, c2};
                auto &pys = spTable_[input];

                std::vector<PinyinInitial> initials;
                std::vector<PinyinFinal> finals;
                auto initialIterPair = initialMap_.equal_range(c1);
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

                if (zeroS_.find(c1) != std::string::npos) {
                    initials.push_back(PinyinInitial::Zero);
                }

                auto finalIterPair = finalMap_.equal_range(c2);
                for (auto &item : boost::make_iterator_range(
                         finalIterPair.first, finalIterPair.second)) {
                    finals.push_back(item.second);
                }

                for (auto i : initials) {
                    for (auto f : finals) {
                        auto py = PinyinEncoder::initialToString(i) +
                                  PinyinEncoder::finalToString(f);
                        addPinyin(pys, py);
                    }
                }

                if (pys.empty()) {
                    spTable_.erase(input);
                }
            }
        }

        // Populate initial final map.
        for (const auto &p : initialFinalMap_) {
            auto &pys = spTable_[p.first];
            auto py = PinyinEncoder::initialToString(p.second.first) +
                      PinyinEncoder::finalToString(p.second.second);
            addPinyin(pys, py);
        }

        // Add non-existent 2 char pinyin to the map.
        for (const auto &p : getPinyinMapV2()) {
            // Don't add "ng" as two char direct pinyin.
            if (p.pinyin() == "ng") {
                continue;
            }

            if (p.pinyin().size() == 2 && p.initial() == PinyinInitial::Zero &&
                (!spTable_.count(p.pinyin()) ||
                 zeroS_.find('*') != std::string::npos)) {
                auto &pys = spTable_[p.pinyin()];
                pys.emplace(PinyinSyllable{p.initial(), p.final()}, p.flags());
            }
        }

        // Add partial pinyin to the table.
        for (char c : validInputs_) {
            std::string input{c};
            auto &pys = spTable_[input];
            auto initial = PinyinEncoder::stringToInitial(std::string{c});
            if (initial != PinyinInitial::Invalid) {
                addPinyinToList(pys, initial, PinyinFinal::Invalid,
                                PinyinFuzzyFlag::None);
            }
            auto initialIterPair = initialMap_.equal_range(c);
            for (auto &item : boost::make_iterator_range(
                     initialIterPair.first, initialIterPair.second)) {
                addPinyinToList(pys, item.second, PinyinFinal::Invalid,
                                PinyinFuzzyFlag::None);
            }

            // Add single char final to partial pinyin.
            auto [begin, end] = finalMap_.equal_range(c);
            for (auto &item : boost::make_iterator_range(begin, end)) {
                const auto final = item.second;
                if (PinyinEncoder::finalToString(final).size() == 1 &&
                    PinyinEncoder::isValidInitialFinal(PinyinInitial::Zero,
                                                       final) &&
                    pys.empty()) {
                    addPinyinToList(pys, PinyinInitial::Zero, final,
                                    PinyinFuzzyFlag::None);
                }
            }

            if (pys.empty()) {
                spTable_.erase(input);
            }
        }

        std::vector<std::tuple<std::string, PinyinSyllable, PinyinFuzzyFlags>>
            newEntries;

        if (correctionProfile != nullptr) {
            auto correctionMap = correctionProfile->correctionMap();
            for (const auto &sp : spTable_) {
                const auto &input = sp.first;
                auto &pys = sp.second;

                for (size_t i = 0; i < input.size(); i++) {
                    auto chr = input[i];
                    auto swap = correctionMap.find(chr);
                    if (swap == correctionMap.end() || swap->second.empty()) {
                        continue;
                    }
                    std::string newInput = input;
                    for (auto sub : swap->second) {
                        newInput[i] = sub;
                        for (const auto &x : pys) {
                            newEntries.emplace_back(
                                newInput, x.first,
                                x.second | PinyinFuzzyFlag::Correction);
                        }
                        newInput[i] = chr;
                    }
                }
            }
        }

        for (const auto &newEntry : newEntries) {
            auto &pys = spTable_[std::get<0>(newEntry)];
            pys.emplace(std::get<1>(newEntry), std::get<2>(newEntry));
        }

        for (const auto &sp : spTable_) {
            assert(!sp.first.empty() && sp.first.size() <= 2);
            validInitials_.insert(sp.first[0]);
        }
    }
};

ShuangpinProfile::ShuangpinProfile(ShuangpinBuiltinProfile profile)
    : ShuangpinProfile::ShuangpinProfile(profile, nullptr) {}

ShuangpinProfile::ShuangpinProfile(std::istream &in)
    : ShuangpinProfile::ShuangpinProfile(in, nullptr) {}

ShuangpinProfile::ShuangpinProfile(
    const ShuangpinProfile &rhs,
    const PinyinCorrectionProfile *correctionProfile)
    : d_ptr(std::make_unique<ShuangpinProfilePrivate>()) {
    FCITX_D();
    d->zeroS_ = rhs.d_ptr->zeroS_;
    d->finalMap_ = rhs.d_ptr->finalMap_;
    d->initialMap_ = rhs.d_ptr->initialMap_;
    d->initialFinalMap_ = rhs.d_ptr->initialFinalMap_;
    d->finalSet_ = rhs.d_ptr->finalSet_;
    d->buildShuangpinTable(correctionProfile);
}

ShuangpinProfile::ShuangpinProfile(
    ShuangpinBuiltinProfile profile,
    const PinyinCorrectionProfile *correctionProfile)
    : d_ptr(std::make_unique<ShuangpinProfilePrivate>()) {
    FCITX_D();
    const SP_C *c = nullptr;
    const SP_S *s = nullptr;
    switch (profile) {
    case ShuangpinBuiltinProfile::Ziranma:
        c = SPMap_C_Ziranma;
        s = SPMap_S_Ziranma;
        d->zeroS_ = "o*";
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
        d->zeroS_ = "o*";
        break;
    case ShuangpinBuiltinProfile::Xiaohe:
        d->zeroS_ = "*";
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

    d->buildShuangpinTable(correctionProfile);
}

ShuangpinProfile::ShuangpinProfile(
    std::istream &in, const PinyinCorrectionProfile *correctionProfile)
    : d_ptr(std::make_unique<ShuangpinProfilePrivate>()) {
    FCITX_D();
    std::string line;
    auto isSpaceCheck = boost::is_any_of(" \n\t\r\v\f");
    bool isDefault = false;
    while (std::getline(in, line)) {
        boost::trim_if(line, isSpaceCheck);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::string_view lineView(line);

        std::string_view option("方案名称=");
        if (boost::starts_with(lineView, option)) {
            std::string name{lineView.substr(option.size())};
            boost::trim_if(name, isSpaceCheck);
            isDefault = (name == "自然码" || name == "微软" || name == "紫光" ||
                         name == "拼音加加" || name == "中文之星" ||
                         name == "智能ABC" || name == "小鹤");
        }

        if (isDefault) {
            continue;
        }

        auto tolowerInPlace = [](std::string &s) {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](char c) { return fcitx::charutils::tolower(c); });
        };

        if (lineView[0] == '=' && lineView.size() > 1) {
            d->zeroS_ = std::string(lineView.substr(1));
            tolowerInPlace(d->zeroS_);
            continue;
        }

        auto equal = lineView.find('=');
        // no '=', or equal at first char, or len(substr after equal) != 1
        if (equal == std::string_view::npos || equal == 0) {
            continue;
        }

        if (equal + 2 == line.size()) {
            std::string pinyin{lineView.substr(0, equal)};
            auto key = fcitx::charutils::tolower(lineView[equal + 1]);
            if (auto final = PinyinEncoder::stringToFinal(pinyin);
                final != PinyinFinal::Invalid) {
                d->finalMap_.emplace(key, final);
            } else if (auto initial = PinyinEncoder::stringToInitial(pinyin);
                       initial != PinyinInitial::Invalid) {
                d->initialMap_.emplace(key, initial);
            }
        } else if (equal + 3 == line.size()) {
            std::string pinyin{lineView.substr(0, equal)};
            std::string key{lineView.substr(equal + 1)};
            tolowerInPlace(key);
            try {
                auto result = PinyinEncoder::encodeFullPinyin(pinyin);
                if (result.size() != 2) {
                    continue;
                }
                d->initialFinalMap_.emplace(
                    key, std::make_pair(static_cast<PinyinInitial>(result[0]),
                                        static_cast<PinyinFinal>(result[1])));
            } catch (...) {
            }
        }
    }

    d->buildShuangpinTable(correctionProfile);
}

FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(ShuangpinProfile)

// moved to ShuangpinProfilePrivate::buildShuangpinTable
void ShuangpinProfile::buildShuangpinTable() {}

const ShuangpinProfile::TableType &ShuangpinProfile::table() const {
    FCITX_D();
    return d->spTable_;
}

const ShuangpinProfile::ValidInputSetType &
ShuangpinProfile::validInput() const {
    FCITX_D();
    return d->validInputs_;
}

const ShuangpinProfile::ValidInputSetType &
ShuangpinProfile::validInitial() const {
    FCITX_D();
    return d->validInitials_;
}
} // namespace libime
