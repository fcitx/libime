/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/pinyin/pinyindata.h"
#include "libime/pinyin/pinyinencoder.h"
#include <cstdint>
#include <fcitx-utils/log.h>
#include <iostream>
#include <iterator>
#include <ostream>
#include <string>
#include <unordered_set>

using namespace libime;

static std::string applyFuzzy(const std::string &str, PinyinFuzzyFlags flags) {
    auto result = str;
    if (flags & PinyinFuzzyFlag::CommonTypo) {
        if (result.ends_with("gn")) {
            result[result.size() - 2] = 'n';
            result[result.size() - 1] = 'g';
        }
        if (result.ends_with("on")) {
            result.push_back('g');
        }
        if (result == "din") {
            result.push_back('g');
        }
        if (result.ends_with("v")) {
            result.back() = 'u';
        }
        if (result.ends_with("ve")) {
            result[result.size() - 2] = 'u';
        }
        if (result.ends_with("van")) {
            result[result.size() - 3] = 'u';
        }
        if (result.ends_with("vang")) {
            result[result.size() - 4] = 'u';
        }
    }

    if (flags & PinyinFuzzyFlag::VE_UE) {
        if (result.ends_with("ue")) {
            result[result.size() - 2] = 'v';
        }
    }

    if (flags & PinyinFuzzyFlag::IAN_IANG) {
        if (result.ends_with("ian")) {
            result.push_back('g');
        } else if (result.ends_with("iang")) {
            result.pop_back();
        }
    } else if (flags & PinyinFuzzyFlag::UAN_UANG) {
        if (result.ends_with("uan")) {
            result.push_back('g');
        } else if (result.ends_with("uang")) {
            result.pop_back();
        }
    } else if (flags & PinyinFuzzyFlag::AN_ANG) {
        if (result.ends_with("an")) {
            result.push_back('g');
        } else if (result.ends_with("ang")) {
            result.pop_back();
        }
    }

    if (flags & PinyinFuzzyFlag::EN_ENG) {
        if (result.ends_with("en")) {
            result.push_back('g');
        } else if (result.ends_with("eng")) {
            result.pop_back();
        }
    }

    if (flags & PinyinFuzzyFlag::IN_ING) {
        if (result.ends_with("in")) {
            result.push_back('g');
        } else if (result.ends_with("ing")) {
            result.pop_back();
        }
    }

    if (flags & PinyinFuzzyFlag::U_OU) {
        if (result.ends_with("ou")) {
            result.pop_back();
            result.back() = 'u';
        } else if (result.ends_with("u")) {
            result.back() = 'o';
            result.push_back('u');
        }
    }

    if (flags & PinyinFuzzyFlag::C_CH) {
        if (result.starts_with("ch")) {
            result.erase(std::next(result.begin()));
        } else if (result.starts_with("c")) {
            result.insert(std::next(result.begin()), 'h');
        }
    }

    if (flags & PinyinFuzzyFlag::S_SH) {
        if (result.starts_with("sh")) {
            result.erase(std::next(result.begin()));
        } else if (result.starts_with("s")) {
            result.insert(std::next(result.begin()), 'h');
        }
    }

    if (flags & PinyinFuzzyFlag::Z_ZH) {
        if (result.starts_with("zh")) {
            result.erase(std::next(result.begin()));
        } else if (result.starts_with("z")) {
            result.insert(std::next(result.begin()), 'h');
        }
    }

    if (flags & PinyinFuzzyFlag::F_H) {
        if (result.starts_with("f")) {
            result.front() = 'h';
        } else if (result.starts_with("h")) {
            result.front() = 'f';
        }
    }

    if (flags & PinyinFuzzyFlag::L_N) {
        if (result.starts_with("l")) {
            result.front() = 'n';
        } else if (result.starts_with("n")) {
            result.front() = 'l';
        }
    }

    if (flags & PinyinFuzzyFlag::L_R) {
        if (result.starts_with("l")) {
            result.front() = 'r';
        } else if (result.starts_with("r")) {
            result.front() = 'l';
        }
    }

    return result;
}

int main() {
    std::unordered_set<std::string> seen;
    for (const auto &p : getPinyinMap()) {
        const auto &pinyin = p.pinyin();
        auto initial = p.initial();
        auto final = p.final();

        auto fullPinyin = PinyinEncoder::initialToString(initial) +
                          PinyinEncoder::finalToString(final);
        FCITX_ASSERT(fullPinyin == applyFuzzy(pinyin, p.flags()));
        if (p.flags() == 0) {
            // make sure valid item is unique
            auto result = seen.insert(pinyin);
            FCITX_ASSERT(result.second);

            int16_t encode =
                ((static_cast<int16_t>(initial) - PinyinEncoder::firstInitial) *
                 (PinyinEncoder::lastFinal - PinyinEncoder::firstFinal + 1)) +
                (static_cast<int16_t>(final) - PinyinEncoder::firstFinal);
            FCITX_ASSERT(PinyinEncoder::isValidInitialFinal(initial, final))
                << " " << encode;
            std::cout << encode << "," << '\n';
        }
    }
    FCITX_ASSERT(getPinyinMapV2().count("zhaung"));
    FCITX_ASSERT(getPinyinMapV2().count("jvn"));
    FCITX_ASSERT(getPinyinMapV2().count("yvn"));
    FCITX_ASSERT(getPinyinMapV2().count("din") > 1);
    return 0;
}
