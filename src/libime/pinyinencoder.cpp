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
#include <unordered_map>

namespace libime {

template <typename L, typename R>
boost::bimap<L, R> makeBimap(std::initializer_list<typename boost::bimap<L, R>::value_type> list) {
    return boost::bimap<L, R>(list.begin(), list.end());
}

static const auto initialMap = makeBimap<PinyinInitial, std::string>({
    {PinyinInitial::B, "b"},   {PinyinInitial::P, "p"}, {PinyinInitial::M, "m"},   {PinyinInitial::F, "f"},
    {PinyinInitial::D, "d"},   {PinyinInitial::T, "t"}, {PinyinInitial::N, "n"},   {PinyinInitial::L, "l"},
    {PinyinInitial::G, "g"},   {PinyinInitial::K, "k"}, {PinyinInitial::H, "h"},   {PinyinInitial::J, "j"},
    {PinyinInitial::Q, "q"},   {PinyinInitial::X, "x"}, {PinyinInitial::ZH, "zh"}, {PinyinInitial::CH, "ch"},
    {PinyinInitial::SH, "sh"}, {PinyinInitial::R, "r"}, {PinyinInitial::Z, "z"},   {PinyinInitial::C, "c"},
    {PinyinInitial::S, "s"},   {PinyinInitial::Y, "y"}, {PinyinInitial::W, "w"},   {PinyinInitial::Zero, ""},
});

static const auto finalMap = makeBimap<PinyinFinal, std::string>({
    {PinyinFinal::A, "a"},       {PinyinFinal::AI, "ai"},   {PinyinFinal::AN, "an"},     {PinyinFinal::ANG, "ang"},
    {PinyinFinal::AO, "ao"},     {PinyinFinal::E, "e"},     {PinyinFinal::EI, "ei"},     {PinyinFinal::EN, "en"},
    {PinyinFinal::ENG, "eng"},   {PinyinFinal::ER, "er"},   {PinyinFinal::O, "o"},       {PinyinFinal::ONG, "ong"},
    {PinyinFinal::OU, "ou"},     {PinyinFinal::I, "i"},     {PinyinFinal::IA, "ia"},     {PinyinFinal::IE, "ie"},
    {PinyinFinal::IAO, "iao"},   {PinyinFinal::IU, "iu"},   {PinyinFinal::IAN, "ian"},   {PinyinFinal::IN, "in"},
    {PinyinFinal::IANG, "iang"}, {PinyinFinal::ING, "ing"}, {PinyinFinal::IONG, "iong"}, {PinyinFinal::U, "u"},
    {PinyinFinal::UA, "ua"},     {PinyinFinal::UO, "uo"},   {PinyinFinal::UAI, "uai"},   {PinyinFinal::UI, "ui"},
    {PinyinFinal::UAN, "uan"},   {PinyinFinal::UN, "un"},   {PinyinFinal::UANG, "uang"}, {PinyinFinal::V, "v"},
    {PinyinFinal::UE, "ue"},     {PinyinFinal::VE, "ve"},   {PinyinFinal::NG, "ng"},     {PinyinFinal::Zero, ""},
});

static const int maxPinyinLength = 6;

template <typename Range>
boost::string_view longestMatch(Range _range, PinyinFuzzyFlags flags) {
    auto range = boost::string_view(&*_range.begin(), _range.size());
    auto &map = getPinyinMap();
    for (; range.size(); range.remove_suffix(1)) {
        auto iterPair = map.equal_range(range);
        if (iterPair.first != iterPair.second) {
            for (auto &item : boost::make_iterator_range(iterPair.first, iterPair.second)) {
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

PinyinSegments PinyinEncoder::parseUserPinyin(const boost::string_view &pinyin, PinyinFuzzyFlags flags) {
    PinyinSegments result(pinyin.to_string());

    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> q;
    q.push(0);

    while (q.size()) {
        size_t top;
        do {
            top = q.top();
            q.pop();
        } while (q.size() && q.top() == top);
        auto iter = std::next(pinyin.begin(), top);
        auto end = pinyin.end();
        if (std::distance(end, iter) > maxPinyinLength) {
            end = iter + maxPinyinLength;
        }
        auto str = longestMatch(boost::make_iterator_range(iter, end), flags);
        result.addNext(top, top + str.size());
        if (top + str.size() < pinyin.size()) {
            q.push(top + str.size());
        }
        if (str.size() > 1 && top + str.size() < pinyin.size()) {
            // pinyin may end with aegimnoruv
            // and may start with abcdefghjklmnopqrstwxyz.
            // the interection is aegmnor, while for m, it only 'm', so don't consider it
            if (str.back() == 'a' || str.back() == 'e' || str.back() == 'g' || str.back() == 'n' || str.back() == 'o' ||
                str.back() == 'r') {
                auto &map = getPinyinMap();
                if (map.find(str.substr(0, str.size() - 1)) != map.end()) {
                    auto end = pinyin.end();
                    iter += str.size() - 1;
                    if (std::distance(end, iter) > maxPinyinLength) {
                        end = iter + maxPinyinLength;
                    }
                    auto nextMatch = longestMatch(boost::make_iterator_range(iter, end), flags);
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

std::vector<char> PinyinEncoder::encodeFullPinyin(const boost::string_view &pinyin) {
    std::vector<std::string> pinyins;
    boost::split(pinyins, pinyin, boost::is_any_of("'"));
    std::vector<char> result;
    result.resize(pinyins.size() * 2 + 1);
    int idx = 0;
    for (const auto &singlePinyin : pinyins) {
        auto &map = getPinyinMap();
        auto iter = map.find(singlePinyin);
        if (iter == map.end() || iter->flags() != PinyinFuzzyFlag::None) {
            throw std::invalid_argument("invalid full pinyin: " + pinyin.to_string());
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

std::string PinyinEncoder::initialToString(libime::PinyinInitial initial) {
    auto iter = initialMap.left.find(initial);
    if (iter != initialMap.left.end()) {
        return iter->second;
    }
    return {};
}

libime::PinyinInitial PinyinEncoder::stringToInitial(const std::string &str) {
    auto iter = initialMap.right.find(str);
    if (iter != initialMap.right.end()) {
        return iter->second;
    }
    return PinyinInitial::Invalid;
}

std::string PinyinEncoder::finalToString(libime::PinyinFinal final) {
    auto iter = finalMap.left.find(final);
    if (iter != finalMap.left.end()) {
        return iter->second;
    }
    return {};
}

libime::PinyinFinal PinyinEncoder::stringToFinal(const std::string &str) {
    auto iter = finalMap.right.find(str);
    if (iter != finalMap.right.end()) {
        return iter->second;
    }
    return {};
}

std::string PinyinEncoder::applyFuzzy(const std::string &str, PinyinFuzzyFlags flags) {
    auto result = str;
    if (flags & PinyinFuzzyFlag::NG_GN) {
        if (boost::algorithm::ends_with(result, "gn")) {
            result[result.size() - 2] = 'n';
            result[result.size() - 1] = 'g';
        }
    }

    if (flags & PinyinFuzzyFlag::V_U) {
        if (boost::algorithm::ends_with(result, "v")) {
            result.back() = 'u';
        }
    }

    if (flags & PinyinFuzzyFlag::VE_UE) {
        if (boost::algorithm::ends_with(result, "ue")) {
            result[result.size() - 2] = 'v';
        }
    }

    if (flags & PinyinFuzzyFlag::IAN_IANG) {
        if (boost::algorithm::ends_with(result, "ian")) {
            result.push_back('g');
        } else if (boost::algorithm::ends_with(result, "iang")) {
            result.pop_back();
        }
    } else if (flags & PinyinFuzzyFlag::UAN_UANG) {
        if (boost::algorithm::ends_with(result, "uan")) {
            result.push_back('g');
        } else if (boost::algorithm::ends_with(result, "uang")) {
            result.pop_back();
        }
    } else if (flags & PinyinFuzzyFlag::AN_ANG) {
        if (boost::algorithm::ends_with(result, "an")) {
            result.push_back('g');
        } else if (boost::algorithm::ends_with(result, "ang")) {
            result.pop_back();
        }
    }

    if (flags & PinyinFuzzyFlag::EN_ENG) {
        if (boost::algorithm::ends_with(result, "en")) {
            result.push_back('g');
        } else if (boost::algorithm::ends_with(result, "eng")) {
            result.pop_back();
        }
    }

    if (flags & PinyinFuzzyFlag::IN_ING) {
        if (boost::algorithm::ends_with(result, "in")) {
            result.push_back('g');
        } else if (boost::algorithm::ends_with(result, "ing")) {
            result.pop_back();
        }
    }

    if (flags & PinyinFuzzyFlag::U_OU) {
        if (boost::algorithm::ends_with(result, "ou")) {
            result.pop_back();
            result.back() = 'u';
        } else if (boost::algorithm::ends_with(result, "u")) {
            result.back() = 'o';
            result.push_back('u');
        }
    }

    if (flags & PinyinFuzzyFlag::C_CH) {
        if (boost::algorithm::starts_with(result, "ch")) {
            result.erase(std::next(result.begin()));
        } else if (boost::algorithm::starts_with(result, "c")) {
            result.insert(std::next(result.begin()), 'h');
            ;
        }
    }

    if (flags & PinyinFuzzyFlag::S_SH) {
        if (boost::algorithm::starts_with(result, "sh")) {
            result.erase(std::next(result.begin()));
        } else if (boost::algorithm::starts_with(result, "s")) {
            result.insert(std::next(result.begin()), 'h');
            ;
        }
    }

    if (flags & PinyinFuzzyFlag::Z_ZH) {
        if (boost::algorithm::starts_with(result, "zh")) {
            result.erase(std::next(result.begin()));
        } else if (boost::algorithm::starts_with(result, "z")) {
            result.insert(std::next(result.begin()), 'h');
            ;
        }
    }

    if (flags & PinyinFuzzyFlag::F_H) {
        if (boost::algorithm::starts_with(result, "f")) {
            result.front() = 'h';
        } else if (boost::algorithm::starts_with(result, "h")) {
            result.front() = 'f';
        }
    }

    if (flags & PinyinFuzzyFlag::L_N) {
        if (boost::algorithm::starts_with(result, "l")) {
            result.front() = 'n';
        } else if (boost::algorithm::starts_with(result, "n")) {
            result.front() = 'l';
        }
    }

    return result;
};
}
