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

#include "libime/pinyin/pinyindata.h"
#include "libime/pinyin/pinyinencoder.h"
#include <boost/algorithm/string.hpp>
#include <fcitx-utils/log.h>
#include <unordered_set>

using namespace libime;

static std::string applyFuzzy(const std::string &str, PinyinFuzzyFlags flags) {
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
}

int main() {
    std::unordered_set<std::string> seen;
    for (auto &p : getPinyinMap()) {
        auto pinyin = p.pinyin();
        auto initial = p.initial();
        auto final = p.final();

        auto fullPinyin = PinyinEncoder::initialToString(initial) +
                          PinyinEncoder::finalToString(final);
        FCITX_ASSERT(fullPinyin == applyFuzzy(pinyin.to_string(), p.flags()));
        if (p.flags() == 0) {
            // make sure valid item is unique
            auto result = seen.insert(pinyin.to_string());
            FCITX_ASSERT(result.second);

            int16_t encode =
                ((static_cast<int16_t>(initial) - PinyinEncoder::firstInitial) *
                 (PinyinEncoder::lastFinal - PinyinEncoder::firstFinal + 1)) +
                (static_cast<int16_t>(final) - PinyinEncoder::firstFinal);
            FCITX_ASSERT(PinyinEncoder::isValidInitialFinal(initial, final));
            std::cout << encode << "," << std::endl;
        }
    }
    return 0;
}
