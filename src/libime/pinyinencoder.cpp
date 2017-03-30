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
#include "pinyinencoder.h"
#include "pinyindata.h"
#include <boost/algorithm/string.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/utility/string_view.hpp>
#include <iostream>
#include <queue>
#include <sstream>
#include <tuple>
#include <unordered_map>

namespace libime {

static const std::string emptyString;

template <typename L, typename R>
boost::bimap<L, R>
makeBimap(std::initializer_list<typename boost::bimap<L, R>::value_type> list) {
    return boost::bimap<L, R>(list.begin(), list.end());
}

static const auto initialMap = makeBimap<PinyinInitial, std::string>({
    {PinyinInitial::B, "b"},   {PinyinInitial::P, "p"},
    {PinyinInitial::M, "m"},   {PinyinInitial::F, "f"},
    {PinyinInitial::D, "d"},   {PinyinInitial::T, "t"},
    {PinyinInitial::N, "n"},   {PinyinInitial::L, "l"},
    {PinyinInitial::G, "g"},   {PinyinInitial::K, "k"},
    {PinyinInitial::H, "h"},   {PinyinInitial::J, "j"},
    {PinyinInitial::Q, "q"},   {PinyinInitial::X, "x"},
    {PinyinInitial::ZH, "zh"}, {PinyinInitial::CH, "ch"},
    {PinyinInitial::SH, "sh"}, {PinyinInitial::R, "r"},
    {PinyinInitial::Z, "z"},   {PinyinInitial::C, "c"},
    {PinyinInitial::S, "s"},   {PinyinInitial::Y, "y"},
    {PinyinInitial::W, "w"},   {PinyinInitial::Zero, ""},
});

static const auto finalMap = makeBimap<PinyinFinal, std::string>({
    {PinyinFinal::A, "a"},       {PinyinFinal::AI, "ai"},
    {PinyinFinal::AN, "an"},     {PinyinFinal::ANG, "ang"},
    {PinyinFinal::AO, "ao"},     {PinyinFinal::E, "e"},
    {PinyinFinal::EI, "ei"},     {PinyinFinal::EN, "en"},
    {PinyinFinal::ENG, "eng"},   {PinyinFinal::ER, "er"},
    {PinyinFinal::O, "o"},       {PinyinFinal::ONG, "ong"},
    {PinyinFinal::OU, "ou"},     {PinyinFinal::I, "i"},
    {PinyinFinal::IA, "ia"},     {PinyinFinal::IE, "ie"},
    {PinyinFinal::IAO, "iao"},   {PinyinFinal::IU, "iu"},
    {PinyinFinal::IAN, "ian"},   {PinyinFinal::IN, "in"},
    {PinyinFinal::IANG, "iang"}, {PinyinFinal::ING, "ing"},
    {PinyinFinal::IONG, "iong"}, {PinyinFinal::U, "u"},
    {PinyinFinal::UA, "ua"},     {PinyinFinal::UO, "uo"},
    {PinyinFinal::UAI, "uai"},   {PinyinFinal::UI, "ui"},
    {PinyinFinal::UAN, "uan"},   {PinyinFinal::UN, "un"},
    {PinyinFinal::UANG, "uang"}, {PinyinFinal::V, "v"},
    {PinyinFinal::UE, "ue"},     {PinyinFinal::VE, "ve"},
    {PinyinFinal::NG, "ng"},     {PinyinFinal::Zero, ""},
});

static const int maxPinyinLength = 6;

template <typename Range>
boost::string_view longestMatch(Range _range, PinyinFuzzyFlags flags) {
    auto range = boost::string_view(&*_range.begin(), _range.size());
    auto &map = getPinyinMap();
    for (; range.size(); range.remove_suffix(1)) {
        auto iterPair = map.equal_range(range);
        if (iterPair.first != iterPair.second) {
            for (auto &item :
                 boost::make_iterator_range(iterPair.first, iterPair.second)) {
                if (flags.test(item.flags())) {
                    return range;
                }
            }
        }
        if (range.size() <= 2) {
            auto iter = initialMap.right.find(range.to_string());
            if (iter != initialMap.right.end()) {
                return range;
            }
        }
    }

    if (!range.size()) {
        range = boost::string_view(&*_range.begin(), 1);
    }

    return range;
}

std::string PinyinSyllable::toString() const {
    return PinyinEncoder::initialToString(initial_) +
           PinyinEncoder::finalToString(final_);
}

SegmentGraph PinyinEncoder::parseUserPinyin(boost::string_view pinyin,
                                            PinyinFuzzyFlags flags) {
    SegmentGraph result(pinyin.to_string());

    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> q;
    q.push(0);

    while (q.size()) {
        size_t top;
        do {
            top = q.top();
            q.pop();
        } while (q.size() && q.top() == top);
        auto iter = std::next(pinyin.begin(), top);
        if (*iter == '\'') {
            while (*iter == '\'' && iter != pinyin.end()) {
                iter++;
            }
            auto next = std::distance(pinyin.begin(), iter);
            result.addNext(top, next);
            if (next < pinyin.size()) {
                q.push(next);
            }
            continue;
        }
        auto end = pinyin.end();
        if (std::distance(iter, end) > maxPinyinLength) {
            end = iter + maxPinyinLength;
        }
        auto str = longestMatch(boost::make_iterator_range(iter, end), flags);
        result.addNext(top, top + str.size());
        if (top + str.size() < pinyin.size()) {
            q.push(top + str.size());
        }

        if (str.size() >= 4 && flags.test(PinyinFuzzyFlag::Inner)) {
            auto &innerSegments = getInnerSegment();
            auto iter = innerSegments.find(str.to_string());
            if (iter != innerSegments.end()) {
                result.addNext(top, top + iter->second.first.size());
                result.addNext(top + iter->second.first.size(),
                               top + str.size());
            }
        }

        if (str.size() > 1 && top + str.size() < pinyin.size()) {
            // pinyin may end with aegimnoruv
            // and may start with abcdefghjklmnopqrstwxyz.
            // the interection is aegmnor, while for m, it only 'm', so don't
            // consider it
            if (str.back() == 'a' || str.back() == 'e' || str.back() == 'g' ||
                str.back() == 'n' || str.back() == 'o' || str.back() == 'r') {
                auto &map = getPinyinMap();
                if (map.find(str.substr(0, str.size() - 1)) != map.end()) {
                    auto end = pinyin.end();
                    iter += str.size() - 1;
                    if (std::distance(end, iter) > maxPinyinLength) {
                        end = iter + maxPinyinLength;
                    }
                    auto nextMatch = longestMatch(
                        boost::make_iterator_range(iter, end), flags);
                    if (nextMatch.size() > 1) {
                        result.addNext(top, top + str.size() - 1);
                        q.push(top + str.size() - 1);
                    }
                }
            }
        }
    }

    return result;
}

std::vector<char> PinyinEncoder::encodeFullPinyin(boost::string_view pinyin) {
    std::vector<std::string> pinyins;
    boost::split(pinyins, pinyin, boost::is_any_of("'"));
    std::vector<char> result;
    result.resize(pinyins.size() * 2 + 1);
    int idx = 0;
    for (const auto &singlePinyin : pinyins) {
        auto &map = getPinyinMap();
        auto iter = map.find(singlePinyin);
        if (iter == map.end() || iter->flags() != PinyinFuzzyFlag::None) {
            throw std::invalid_argument("invalid full pinyin: " +
                                        pinyin.to_string());
        }
        result[idx] = static_cast<char>(iter->initial());
        result[pinyins.size() + idx + 1] = static_cast<char>(iter->final());
        idx++;
    }
    result[pinyins.size()] = initialFinalSepartor;

    return result;
}

std::string PinyinEncoder::decodeFullPinyin(const char *data, size_t size) {
    if (size % 2 != 1 || data[size / 2] != initialFinalSepartor) {
        throw std::invalid_argument("invalid pinyin key");
    }
    std::stringstream result;
    for (size_t i = 0, e = size / 2; i < e; i++) {
        if (i) {
            result << '\'';
        }
        result << initialToString(static_cast<PinyinInitial>(data[i]));
        result << finalToString(static_cast<PinyinFinal>(data[i + e + 1]));
    }
    return result.str();
}

const std::string &PinyinEncoder::initialToString(PinyinInitial initial) {
    auto iter = initialMap.left.find(initial);
    if (iter != initialMap.left.end()) {
        return iter->second;
    }
    return emptyString;
}

PinyinInitial PinyinEncoder::stringToInitial(const std::string &str) {
    auto iter = initialMap.right.find(str);
    if (iter != initialMap.right.end()) {
        return iter->second;
    }
    return PinyinInitial::Invalid;
}

const std::string &PinyinEncoder::finalToString(PinyinFinal final) {
    auto iter = finalMap.left.find(final);
    if (iter != finalMap.left.end()) {
        return iter->second;
    }
    return emptyString;
}

PinyinFinal PinyinEncoder::stringToFinal(const std::string &str) {
    auto iter = finalMap.right.find(str);
    if (iter != finalMap.right.end()) {
        return iter->second;
    }
    return {};
}

bool PinyinEncoder::isValidInitialFinal(PinyinInitial initial,
                                        PinyinFinal final) {
    if (initial != PinyinInitial::Invalid && final != PinyinFinal::Invalid) {
        int16_t encode =
            ((static_cast<int16_t>(initial) - PinyinEncoder::firstInitial) *
             (PinyinEncoder::lastInitial - PinyinEncoder::firstInitial + 1)) +
            (static_cast<int16_t>(final) - PinyinEncoder::firstFinal);
        return getEncodedInitialFinal().count(encode) != 0;
    }
    return false;
}

static void
getFuzzy(std::vector<std::pair<PinyinInitial, std::vector<PinyinFinal>>> &syls,
         PinyinSyllable syl, PinyinFuzzyFlags flags) {
    // ng/gn is already handled by table
    PinyinInitial initials[2] = {syl.initial(), PinyinInitial::Invalid};
    PinyinFinal finals[2] = {syl.final(), PinyinFinal::Invalid};
    int initialSize = 1;
    int finalSize = 1;

    // for {s,z,c} we also want them to match {sh,zh,ch}
    if (syl.final() == PinyinFinal::Zero) {
        flags |= PinyinFuzzyFlag::C_CH;
        flags |= PinyinFuzzyFlag::Z_ZH;
        flags |= PinyinFuzzyFlag::S_SH;
    }

    const static std::vector<
        std::tuple<PinyinInitial, PinyinInitial, PinyinFuzzyFlag>>
        initialFuzzies = {
            {PinyinInitial::C, PinyinInitial::CH, PinyinFuzzyFlag::C_CH},
            {PinyinInitial::S, PinyinInitial::SH, PinyinFuzzyFlag::S_SH},
            {PinyinInitial::Z, PinyinInitial::Z, PinyinFuzzyFlag::Z_ZH},
            {PinyinInitial::F, PinyinInitial::H, PinyinFuzzyFlag::F_H},
            {PinyinInitial::L, PinyinInitial::N, PinyinFuzzyFlag::L_N},
        };

    for (auto &initialFuzzy : initialFuzzies) {
        if ((syl.initial() == std::get<0>(initialFuzzy) ||
             syl.initial() == std::get<1>(initialFuzzy)) &&
            flags & std::get<2>(initialFuzzy)) {
            initials[1] = syl.initial() == std::get<0>(initialFuzzy)
                              ? std::get<1>(initialFuzzy)
                              : std::get<0>(initialFuzzy);
            initialSize = 2;
            break;
        }
    }

    const static std::vector<
        std::tuple<PinyinFinal, PinyinFinal, PinyinFuzzyFlag>>
        finalFuzzies = {
            {PinyinFinal::V, PinyinFinal::U, PinyinFuzzyFlag::V_U},
            {PinyinFinal::AN, PinyinFinal::ANG, PinyinFuzzyFlag::AN_ANG},
            {PinyinFinal::EN, PinyinFinal::ENG, PinyinFuzzyFlag::EN_ENG},
            {PinyinFinal::IAN, PinyinFinal::IANG, PinyinFuzzyFlag::IAN_IANG},
            {PinyinFinal::IN, PinyinFinal::ING, PinyinFuzzyFlag::IN_ING},
            {PinyinFinal::U, PinyinFinal::OU, PinyinFuzzyFlag::U_OU},
            {PinyinFinal::UAN, PinyinFinal::UANG, PinyinFuzzyFlag::UAN_UANG},
            {PinyinFinal::VE, PinyinFinal::UE, PinyinFuzzyFlag::VE_UE},
        };

    for (auto &finalFuzzy : finalFuzzies) {
        if ((syl.final() == std::get<0>(finalFuzzy) ||
             syl.final() == std::get<1>(finalFuzzy)) &&
            flags & std::get<2>(finalFuzzy)) {
            finals[1] = syl.final() == std::get<0>(finalFuzzy)
                            ? std::get<1>(finalFuzzy)
                            : std::get<0>(finalFuzzy);
            finalSize = 2;
            break;
        }
    }

    for (int i = 0; i < initialSize; i++) {
        for (int j = 0; j < finalSize; j++) {
            auto initial = initials[i];
            auto final = finals[j];
            if ((i == 0 && j == 0) || final == PinyinFinal::Invalid ||
                PinyinEncoder::isValidInitialFinal(initial, final)) {
                auto iter = std::find_if(
                    syls.begin(), syls.end(),
                    [initial](const auto &p) { return p.first == initial; });
                if (iter == syls.end()) {
                    syls.emplace_back(std::piecewise_construct,
                                      std::forward_as_tuple(initial),
                                      std::forward_as_tuple());
                    iter = std::prev(syls.end());
                }
                auto &finals = iter->second;
                if (std::find(finals.begin(), finals.end(), final) ==
                    finals.end()) {
                    finals.push_back(final);
                }
            }
        }
    }
}

std::vector<std::pair<PinyinInitial, std::vector<PinyinFinal>>>
PinyinEncoder::stringToSyllables(boost::string_view pinyin,
                                 PinyinFuzzyFlags flags) {
    std::vector<std::pair<PinyinInitial, std::vector<PinyinFinal>>> result;
    auto &map = getPinyinMap();
    // we only want {M,N,R}/Invalid instead of {M,N,R}/Zero, so we could get
    // match for everything.
    if (pinyin != "m" && pinyin != "n" && pinyin != "r") {
        auto iterPair = map.equal_range(pinyin);
        for (auto &item :
             boost::make_iterator_range(iterPair.first, iterPair.second)) {
            if (flags.test(item.flags())) {
                getFuzzy(result, {item.initial(), item.final()}, flags);
            }
        }
    }

    auto iter = initialMap.right.find(pinyin.to_string());
    if (initialMap.right.end() != iter) {
        getFuzzy(result, {iter->second, PinyinFinal::Invalid}, flags);
    }

    if (result.size() == 0) {
        result.emplace_back(std::piecewise_construct,
                            std::forward_as_tuple(PinyinInitial::Invalid),
                            std::forward_as_tuple(1, PinyinFinal::Invalid));
    }

    return result;
}
}
