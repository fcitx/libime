/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "pinyinencoder.h"
#include "pinyindata.h"
#include "shuangpinprofile.h"
#include <boost/algorithm/string.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <fcitx-utils/charutils.h>
#include <queue>
#include <sstream>
#include <string_view>
#include <tuple>
#include <unordered_map>

namespace libime {

static const std::string emptyString;

fcitx::LogMessageBuilder &operator<<(fcitx::LogMessageBuilder &log,
                                     PinyinInitial initial) {
    log << PinyinEncoder::initialToString(initial);
    return log;
}

fcitx::LogMessageBuilder &operator<<(fcitx::LogMessageBuilder &log,
                                     PinyinFinal final) {
    log << PinyinEncoder::finalToString(final);
    return log;
}

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

template <typename Iter>
std::pair<std::string_view, bool> longestMatch(Iter iter, Iter end,
                                               PinyinFuzzyFlags flags) {
    if (std::distance(iter, end) > maxPinyinLength) {
        end = iter + maxPinyinLength;
    }
    auto range = std::string_view(&*iter, std::distance(iter, end));
    const auto &map = getPinyinMap();
    for (; !range.empty(); range.remove_suffix(1)) {
        auto iterPair = map.equal_range(range);
        if (iterPair.first != iterPair.second) {
            for (const auto &item :
                 boost::make_iterator_range(iterPair.first, iterPair.second)) {
                if (flags.test(item.flags())) {
                    // do not consider m/n/r as complete pinyin
                    return std::make_pair(
                        range, (range != "m" && range != "n" && range != "r"));
                }
            }
        }
        if (range.size() <= 2) {
            auto iter = initialMap.right.find(std::string{range});
            if (iter != initialMap.right.end()) {
                return std::make_pair(range, false);
            }
        }
    }

    if (range.empty()) {
        range = std::string_view(&*iter, 1);
    }

    return std::make_pair(range, false);
}

std::string PinyinSyllable::toString() const {
    return PinyinEncoder::initialToString(initial_) +
           PinyinEncoder::finalToString(final_);
}

SegmentGraph PinyinEncoder::parseUserPinyin(std::string userPinyin,
                                            PinyinFuzzyFlags flags) {
    SegmentGraph result{std::move(userPinyin)};
    auto pinyin = result.data();
    std::transform(pinyin.begin(), pinyin.end(), pinyin.begin(),
                   fcitx::charutils::tolower);
    auto end = pinyin.end();
    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> q;
    q.push(0);
    while (!q.empty()) {
        size_t top;
        do {
            top = q.top();
            q.pop();
        } while (!q.empty() && q.top() == top);
        if (top >= pinyin.size()) {
            continue;
        }
        auto iter = std::next(pinyin.begin(), top);
        if (*iter == '\'') {
            while (*iter == '\'' && iter != pinyin.end()) {
                iter++;
            }
            auto next = std::distance(pinyin.begin(), iter);
            result.addNext(top, next);
            if (static_cast<size_t>(next) < pinyin.size()) {
                q.push(next);
            }
            continue;
        }
        std::string_view str;
        bool isCompletePinyin;
        std::tie(str, isCompletePinyin) = longestMatch(iter, end, flags);

        // it's not complete a pinyin, no need to try
        if (!isCompletePinyin) {
            result.addNext(top, top + str.size());
            q.push(top + str.size());
        } else {
            // check fuzzy seg
            // pinyin may end with aegimnoruv
            // and may start with abcdefghjklmnopqrstwxyz.
            // the interection is aegmnor, while for m, it only 'm', so don't
            // consider it
            // also, make sure current pinyin does not end with a separator,
            // other wise, jin'an may be parsed into ji'n because, nextMatch is
            // starts with "'".
            const auto &map = getPinyinMap();
            std::array<size_t, 2> nextSize;
            size_t nNextSize = 0;
            if (str.size() > 1 && top + str.size() < pinyin.size() &&
                pinyin[top + str.size()] != '\'' &&
                (str.back() == 'a' || str.back() == 'e' || str.back() == 'g' ||
                 str.back() == 'n' || str.back() == 'o' || str.back() == 'r') &&
                map.find(str.substr(0, str.size() - 1)) != map.end()) {
                // str[0:-1] is also a full pinyin, check next pinyin
                auto nextMatch = longestMatch(iter + str.size(), end, flags);
                auto nextMatchAlt =
                    longestMatch(iter + str.size() - 1, end, flags);
                auto matchSize = str.size() + nextMatch.first.size();
                auto matchSizeAlt = str.size() - 1 + nextMatchAlt.first.size();
                if (std::make_pair(matchSize, nextMatch.second) >=
                    std::make_pair(matchSizeAlt, nextMatchAlt.second)) {
                    result.addNext(top, top + str.size());
                    q.push(top + str.size());
                    nextSize[nNextSize++] = str.size();
                }
                if (std::make_pair(matchSize, nextMatch.second) <=
                    std::make_pair(matchSizeAlt, nextMatchAlt.second)) {
                    result.addNext(top, top + str.size() - 1);
                    q.push(top + str.size() - 1);
                    nextSize[nNextSize++] = str.size() - 1;
                }
            } else {
                result.addNext(top, top + str.size());
                q.push(top + str.size());
                nextSize[nNextSize++] = str.size();
            }

            for (size_t i = 0; i < nNextSize; i++) {
                if (nextSize[i] >= 3 && flags.test(PinyinFuzzyFlag::Inner)) {
                    const auto &innerSegments = getInnerSegment();
                    auto iter = innerSegments.find(
                        std::string{str.substr(0, nextSize[i])});
                    if (iter != innerSegments.end()) {
                        result.addNext(top, top + iter->second.first.size());
                        result.addNext(top + iter->second.first.size(),
                                       top + nextSize[i]);
                    }
                }
            }
        }
    }
    return result;
}

SegmentGraph PinyinEncoder::parseUserShuangpin(std::string userPinyin,
                                               const ShuangpinProfile &sp,
                                               PinyinFuzzyFlags flags) {
    SegmentGraph result{std::move(userPinyin)};
    auto pinyin = result.data();
    std::transform(pinyin.begin(), pinyin.end(), pinyin.begin(),
                   fcitx::charutils::tolower);

    // assume user always type valid shuangpin first, if not keep one.
    size_t i = 0;

    const auto &table = sp.table();
    while (i < pinyin.size()) {
        auto start = i;
        while (pinyin[i] == '\'' && i < pinyin.size()) {
            i++;
        }
        if (start != i) {
            result.addNext(start, i);
            continue;
        }
        auto initial = pinyin[i];
        char final = '\0';
        if (i + 1 < pinyin.size() && pinyin[i + 1] != '\'') {
            final = pinyin[i + 1];
        }

        std::string match{initial};
        if (final) {
            match.push_back(final);
        }

        auto longestMatchInTable = [flags](decltype(table) t,
                                           const std::string &v) {
            auto py = v;
            while (!py.empty()) {
                auto iter = t.find(py);
                if (iter != t.end()) {
                    for (const auto &p : iter->second) {
                        if (flags.test(p.second)) {
                            return iter;
                        }
                    }
                }
                py.pop_back();
            }
            return t.end();
        };

        auto iter = longestMatchInTable(table, match);
        if (iter != table.end()) {
            result.addNext(i, i + iter->first.size());
            i = i + iter->first.size();
        } else {
            result.addNext(i, i + 1);
            i = i + 1;
        }
    }

    return result;
}

std::vector<char> PinyinEncoder::encodeFullPinyin(std::string_view pinyin) {
    std::vector<std::string> pinyins;
    boost::split(pinyins, pinyin, boost::is_any_of("'"));
    std::vector<char> result;
    result.resize(pinyins.size() * 2);
    int idx = 0;
    for (const auto &singlePinyin : pinyins) {
        const auto &map = getPinyinMap();
        auto iter = map.find(singlePinyin);
        if (iter == map.end() || iter->flags() != PinyinFuzzyFlag::None) {
            throw std::invalid_argument("invalid full pinyin: " +
                                        std::string{pinyin});
        }
        result[idx++] = static_cast<char>(iter->initial());
        result[idx++] = static_cast<char>(iter->final());
    }

    return result;
}

std::vector<char> PinyinEncoder::encodeOneUserPinyin(std::string pinyin) {
    if (pinyin.empty()) {
        return {};
    }
    auto graph = parseUserPinyin(std::move(pinyin), PinyinFuzzyFlag::None);
    std::vector<char> result;
    const SegmentGraphNode *node = &graph.start(), *prev = nullptr;
    while (node->nextSize()) {
        prev = node;
        node = &node->nexts().front();
        auto seg = graph.segment(*prev, *node);
        if (seg.empty() || seg[0] == '\'') {
            continue;
        }
        auto syls = stringToSyllables(seg, PinyinFuzzyFlag::None);
        if (syls.empty()) {
            return {};
        }
        result.push_back(static_cast<char>(syls[0].first));
        result.push_back(static_cast<char>(syls[0].second[0].first));
    }
    return result;
}

bool PinyinEncoder::isValidUserPinyin(const char *data, size_t size) {
    if (size % 2 != 0) {
        return false;
    }

    for (size_t i = 0; i < size / 2; i++) {
        if (!PinyinEncoder::isValidInitial(data[i * 2])) {
            return false;
        }
    }
    return true;
}

std::string PinyinEncoder::decodeFullPinyin(const char *data, size_t size) {
    if (size % 2 != 0) {
        throw std::invalid_argument("invalid pinyin key");
    }
    std::string result;
    for (size_t i = 0, e = size / 2; i < e; i++) {
        if (i) {
            result += '\'';
        }
        result += initialToString(static_cast<PinyinInitial>(data[i * 2]));
        result += finalToString(static_cast<PinyinFinal>(data[i * 2 + 1]));
    }
    return result;
}

const std::string &PinyinEncoder::initialToString(PinyinInitial initial) {
    const static std::vector<std::string> s = []() {
        std::vector<std::string> s;
        s.resize(lastInitial - firstInitial + 1);
        for (char c = firstInitial; c <= lastInitial; c++) {
            auto iter = initialMap.left.find(static_cast<PinyinInitial>(c));
            s[c - firstInitial] = iter->second;
        }
        return s;
    }();
    auto c = static_cast<char>(initial);
    if (c >= firstInitial && c <= lastInitial) {
        return s[c - firstInitial];
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
    const static std::vector<std::string> s = []() {
        std::vector<std::string> s;
        s.resize(lastFinal - firstFinal + 1);
        for (char c = firstFinal; c <= lastFinal; c++) {
            auto iter = finalMap.left.find(static_cast<PinyinFinal>(c));
            s[c - firstFinal] = iter->second;
        }
        return s;
    }();
    auto c = static_cast<char>(final);
    if (c >= firstFinal && c <= lastFinal) {
        return s[c - firstFinal];
    }
    return emptyString;
}

PinyinFinal PinyinEncoder::stringToFinal(const std::string &str) {
    auto iter = finalMap.right.find(str);
    if (iter != finalMap.right.end()) {
        return iter->second;
    }
    return PinyinFinal::Invalid;
}

bool PinyinEncoder::isValidInitialFinal(PinyinInitial initial,
                                        PinyinFinal final) {
    if (initial != PinyinInitial::Invalid && final != PinyinFinal::Invalid) {
        int16_t encode =
            ((static_cast<int16_t>(initial) - PinyinEncoder::firstInitial) *
             (PinyinEncoder::lastFinal - PinyinEncoder::firstFinal + 1)) +
            (static_cast<int16_t>(final) - PinyinEncoder::firstFinal);
        const auto &a = getEncodedInitialFinal();
        return encode < static_cast<int>(a.size()) && a[encode];
    }
    return false;
}

std::string PinyinEncoder::initialFinalToPinyinString(PinyinInitial initial,
                                                      PinyinFinal final) {
    std::string result = initialToString(initial);
    std::string finalString;
    switch (final) {
    case PinyinFinal::VE:
    case PinyinFinal::V:
        if (initial == PinyinInitial::N || initial == PinyinInitial::L) {
            if (final == PinyinFinal::VE) {
                finalString = "üe";
            } else {
                finalString = "ü";
            }
            break;
        }
        // FALLTHROUGH
    default:
        finalString = finalToString(final);
        break;
    }
    result.append(finalString);
    return result;
}

static void getFuzzy(
    std::vector<std::pair<PinyinInitial,
                          std::vector<std::pair<PinyinFinal, bool>>>> &syls,
    PinyinSyllable syl, PinyinFuzzyFlags flags) {
    // ng/gn is already handled by table
    PinyinInitial initials[2] = {syl.initial(), PinyinInitial::Invalid};
    PinyinFinal finals[2] = {syl.final(), PinyinFinal::Invalid};
    int initialSize = 1;
    int finalSize = 1;

    // for {s,z,c} we also want them to match {sh,zh,ch}
    if (syl.final() == PinyinFinal::Invalid) {
        if (syl.initial() == PinyinInitial::C) {
            flags |= PinyinFuzzyFlag::C_CH;
        }
        if (syl.initial() == PinyinInitial::Z) {
            flags |= PinyinFuzzyFlag::Z_ZH;
        }
        if (syl.initial() == PinyinInitial::S) {
            flags |= PinyinFuzzyFlag::S_SH;
        }
    }

    const static std::vector<
        std::tuple<PinyinInitial, PinyinInitial, PinyinFuzzyFlag>>
        initialFuzzies = {
            {PinyinInitial::C, PinyinInitial::CH, PinyinFuzzyFlag::C_CH},
            {PinyinInitial::S, PinyinInitial::SH, PinyinFuzzyFlag::S_SH},
            {PinyinInitial::Z, PinyinInitial::ZH, PinyinFuzzyFlag::Z_ZH},
            {PinyinInitial::F, PinyinInitial::H, PinyinFuzzyFlag::F_H},
            {PinyinInitial::L, PinyinInitial::N, PinyinFuzzyFlag::L_N},
        };

    for (const auto &initialFuzzy : initialFuzzies) {
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

    for (const auto &finalFuzzy : finalFuzzies) {
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
                if (std::find_if(finals.begin(), finals.end(),
                                 [final](auto &p) {
                                     return p.first == final;
                                 }) == finals.end()) {
                    finals.emplace_back(final, i > 0 || j > 0);
                }
            }
        }
    }
}

MatchedPinyinSyllables
PinyinEncoder::stringToSyllables(std::string_view pinyinView,
                                 PinyinFuzzyFlags flags) {
    std::vector<
        std::pair<PinyinInitial, std::vector<std::pair<PinyinFinal, bool>>>>
        result;
    std::string pinyin(pinyinView);
    std::transform(pinyin.begin(), pinyin.end(), pinyin.begin(),
                   fcitx::charutils::tolower);
    const auto &map = getPinyinMap();
    // we only want {M,N,R}/Invalid instead of {M,N,R}/Zero, so we could get
    // match for everything.
    if (pinyin != "m" && pinyin != "n" && pinyin != "r") {
        auto iterPair = map.equal_range(pinyin);
        for (const auto &item :
             boost::make_iterator_range(iterPair.first, iterPair.second)) {
            if (flags.test(item.flags())) {
                getFuzzy(result, {item.initial(), item.final()}, flags);
            }
        }
    }

    auto iter = initialMap.right.find(std::string{pinyin});
    if (initialMap.right.end() != iter) {
        getFuzzy(result, {iter->second, PinyinFinal::Invalid}, flags);
    }

    if (result.empty()) {
        result.emplace_back(
            std::piecewise_construct,
            std::forward_as_tuple(PinyinInitial::Invalid),
            std::forward_as_tuple(1,
                                  std::make_pair(PinyinFinal::Invalid, false)));
    }

#if 0
    else {
        // replace invalid
        for (auto &p : result) {
            if (p.second.size() == 1 && p.second[0] == PinyinFinal::Invalid) {
                p.second.clear();
                for (char test = PinyinEncoder::firstFinal;
                     test <= PinyinEncoder::lastFinal; test++) {
                    auto final = static_cast<PinyinFinal>(test);
                    if (PinyinEncoder::isValidInitialFinal(p.first, final)) {
                        p.second.push_back(final);
                    }
                }
            }
        }
    }
#endif

    return result;
}

MatchedPinyinSyllables
PinyinEncoder::shuangpinToSyllables(std::string_view pinyinView,
                                    const ShuangpinProfile &sp,
                                    PinyinFuzzyFlags flags) {
    assert(pinyinView.size() <= 2);
    std::string pinyin(pinyinView);
    std::transform(pinyin.begin(), pinyin.end(), pinyin.begin(),
                   fcitx::charutils::tolower);
    const auto &table = sp.table();
    auto iter = table.find(pinyin);

    std::vector<
        std::pair<PinyinInitial, std::vector<std::pair<PinyinFinal, bool>>>>
        result;
    if (iter != table.end()) {
        for (const auto &p : iter->second) {
            if (flags.test(p.second)) {
                getFuzzy(result, {p.first.initial(), p.first.final()}, flags);
            }
        }
    }

    if (result.empty()) {
        result.emplace_back(
            std::piecewise_construct,
            std::forward_as_tuple(PinyinInitial::Invalid),
            std::forward_as_tuple(1,
                                  std::make_pair(PinyinFinal::Invalid, false)));
    }

    return result;
}

std::string
PinyinEncoder::shuangpinToPinyin(std::string_view pinyinView,
                                 const libime::ShuangpinProfile &sp) {
    assert(pinyinView.size() <= 2);
    auto syls = shuangpinToSyllables(pinyinView, sp, PinyinFuzzyFlag::None);
    if (!syls.empty() && !syls[0].second.empty() && !syls[0].second[0].second) {
        auto initial = syls[0].first;
        auto final = syls[0].second[0].first;
        return initialToString(initial) + finalToString(final);
    }
    return "";
}

} // namespace libime
