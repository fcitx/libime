/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "pinyindata.h"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <fcitx-utils/log.h>
#include <fcitx-utils/stringutils.h>
#include "pinyinencoder.h"

namespace libime {

const std::vector<bool> &getEncodedInitialFinal() {
    static const auto encodedInitialFinal = []() {
        std::vector<bool> a;
        const std::unordered_set<int16_t> encodedInitialFinalSet = {
            660, 241, 384, 481, 388, 409, 415, 326, 497, 425, 327, 329, 220,
            331, 55,  332, 336, 350, 352, 253, 43,  255, 799, 256, 417, 257,
            272, 268, 567, 269, 353, 224, 264, 144, 36,  448, 277, 271, 217,
            283, 107, 72,  73,  74,  75,  78,  79,  533, 450, 275, 254, 115,
            80,  85,  90,  182, 583, 360, 87,  91,  237, 330, 95,  77,  410,
            605, 221, 10,  727, 222, 542, 335, 862, 234, 236, 232, 231, 196,
            785, 233, 347, 239, 245, 247, 158, 838, 840, 733, 38,  652, 76,
            44,  619, 162, 328, 228, 18,  49,  54,  51,  218, 37,  52,  57,
            46,  447, 198, 424, 449, 460, 455, 251, 465, 461, 614, 615, 160,
            390, 616, 413, 532, 775, 416, 7,   802, 267, 349, 484, 367, 648,
            620, 798, 756, 262, 589, 485, 280, 548, 419, 749, 386, 451, 411,
            649, 759, 496, 564, 625, 656, 59,  219, 777, 508, 201, 490, 227,
            659, 445, 487, 745, 606, 793, 209, 676, 270, 324, 312, 768, 348,
            795, 184, 767, 482, 834, 314, 412, 512, 654, 454, 815, 639, 223,
            603, 761, 244, 385, 88,  675, 486, 723, 289, 637, 779, 517, 292,
            651, 6,   452, 527, 183, 784, 661, 325, 155, 835, 429, 769, 89,
            207, 789, 483, 653, 195, 724, 489, 229, 551, 531, 576, 171, 792,
            760, 260, 581, 208, 453, 623, 677, 530, 266, 418, 650, 2,   515,
            516, 351, 510, 509, 265, 671, 501, 281, 504, 235, 636, 655, 534,
            640, 528, 186, 511, 641, 263, 371, 582, 387, 553, 721, 588, 673,
            506, 541, 203, 192, 40,  191, 617, 193, 758, 188, 794, 216, 185,
            248, 258, 584, 748, 505, 361, 725, 766, 488, 48,  82,  93,  414,
            149, 491, 181, 284, 180, 577, 600, 747, 159, 205, 743, 732, 731,
            13,  800, 836, 601, 42,  728, 604, 579, 84,  16,  602, 599, 580,
            354, 252, 711, 722, 39,  720, 389, 647, 624, 578, 507, 273, 635,
            529, 446, 383, 317, 372, 368, 366, 365, 364, 566, 363, 362, 313,
            315, 685, 318, 316, 3,   311, 300, 299, 296, 295, 294, 543, 293,
            291, 290, 288, 15,  131, 120, 118, 124, 156, 116, 114, 111, 110,
            108, 837, 833, 169, 173, 172, 167, 161, 165, 157, 152, 151, 150,
            687, 148, 147, 146, 145, 709, 713, 4,   712, 707, 696, 695, 697,
            565, 569, 570, 568, 563, 552, 547, 545, 544, 540, 692, 691, 689,
            688, 686, 684, 23,  21,  19,  8,   1,   0,   832, 831, 830, 829,
            828, 230, 20,  163, 803, 11};
        a.resize(900);
        std::fill(a.begin(), a.end(), false);
        for (auto i : encodedInitialFinalSet) {
            a[i] = true;
        }
        return a;
    }();

    return encodedInitialFinal;
}

const std::unordered_map<std::string, std::pair<std::string, std::string>> &
getInnerSegment() {
    static const std::unordered_map<std::string,
                                    std::pair<std::string, std::string>>
        innerSegment = {
            {"zuo", {"zu", "o"}},       {"zao", {"za", "o"}},
            {"yue", {"yu", "e"}},       {"yve", {"yv", "e"}},
            {"yao", {"ya", "o"}},       {"xue", {"xu", "e"}},
            {"xve", {"xv", "e"}},       {"xie", {"xi", "e"}},
            {"xia", {"xi", "a"}},       {"tuo", {"tu", "o"}},
            {"tie", {"ti", "e"}},       {"tao", {"ta", "o"}},
            {"suo", {"su", "o"}},       {"sao", {"sa", "o"}},
            {"rua", {"ru", "a"}},       {"ruo", {"ru", "o"}},
            {"que", {"qu", "e"}},       {"qve", {"qv", "e"}},
            {"qie", {"qi", "e"}},       {"qia", {"qi", "a"}},
            {"pie", {"pi", "e"}},       {"pao", {"pa", "o"}},
            {"nve", {"nv", "e"}},       {"nue", {"nu", "e"}},
            {"nuo", {"nu", "o"}},       {"nie", {"ni", "e"}},
            {"nao", {"na", "o"}},       {"mie", {"mi", "e"}},
            {"mao", {"ma", "o"}},       {"lve", {"lv", "e"}},
            {"lue", {"lu", "e"}},       {"luo", {"lu", "o"}},
            {"lie", {"li", "e"}},       {"lia", {"li", "a"}},
            {"lao", {"la", "o"}},       {"kuo", {"ku", "o"}},
            {"kua", {"ku", "a"}},       {"kao", {"ka", "o"}},
            {"jue", {"ju", "e"}},       {"jve", {"jv", "e"}},
            {"jie", {"ji", "e"}},       {"jia", {"ji", "a"}},
            {"huo", {"hu", "o"}},       {"hua", {"hu", "a"}},
            {"hao", {"ha", "o"}},       {"guo", {"gu", "o"}},
            {"gua", {"gu", "a"}},       {"gao", {"ga", "o"}},
            {"duo", {"du", "o"}},       {"die", {"di", "e"}},
            {"dia", {"di", "a"}},       {"dao", {"da", "o"}},
            {"cuo", {"cu", "o"}},       {"cao", {"ca", "o"}},
            {"bie", {"bi", "e"}},       {"bao", {"ba", "o"}},
            {"nia", {"ni", "a"}},

            {"xiao", {"xi", "ao"}},     {"xiang", {"xi", "ang"}},
            {"xian", {"xi", "an"}},     {"jiao", {"ji", "ao"}},
            {"jiang", {"ji", "ang"}},   {"jian", {"ji", "an"}},
            {"luan", {"lu", "an"}},     {"miao", {"mi", "ao"}},
            {"mian", {"mi", "an"}},     {"kuang", {"ku", "ang"}},
            {"kuan", {"ku", "an"}},     {"kuai", {"ku", "ai"}},
            {"nuan", {"nu", "an"}},     {"piao", {"pi", "ao"}},
            {"pian", {"pi", "an"}},     {"quan", {"qu", "an"}},
            {"quang", {"qu", "ang"}},   {"qvan", {"qv", "an"}},
            {"qvang", {"qv", "ang"}},   {"juan", {"ju", "an"}},
            {"juang", {"ju", "ang"}},   {"jvan", {"jv", "an"}},
            {"jvang", {"jv", "ang"}},   {"qiao", {"qi", "ao"}},
            {"qiang", {"qi", "ang"}},   {"qian", {"qi", "an"}},
            {"yuang", {"yu", "ang"}},   {"yvang", {"yv", "ang"}},
            {"yuan", {"yu", "an"}},     {"yvan", {"yv", "an"}},
            {"zhuang", {"zhu", "ang"}}, {"zhuan", {"zhu", "an"}},
            {"zhuai", {"zhu", "ai"}},   {"niao", {"ni", "ao"}},
            {"niang", {"ni", "ang"}},   {"nian", {"ni", "an"}},
            {"liao", {"li", "ao"}},     {"liang", {"li", "ang"}},
            {"lian", {"li", "an"}},     {"zuan", {"zu", "an"}},
            {"tuan", {"tu", "an"}},     {"tiao", {"ti", "ao"}},
            {"tian", {"ti", "an"}},     {"xuang", {"xu", "ang"}},
            {"xvang", {"xv", "ang"}},   {"xuan", {"xu", "an"}},
            {"xvan", {"xv", "an"}},     {"suan", {"su", "an"}},
            {"biao", {"bi", "ao"}},     {"bian", {"bi", "an"}},
            {"shuang", {"shu", "ang"}}, {"shuan", {"shu", "an"}},
            {"shuai", {"shu", "ai"}},   {"ruan", {"ru", "an"}},
            {"huang", {"hu", "ang"}},   {"huan", {"hu", "an"}},
            {"huai", {"hu", "ai"}},     {"guang", {"gu", "ang"}},
            {"guan", {"gu", "an"}},     {"guai", {"gu", "ai"}},
            {"duan", {"du", "an"}},     {"diao", {"di", "ao"}},
            {"dian", {"di", "an"}},     {"cuan", {"cu", "an"}},
            {"chuang", {"chu", "ang"}}, {"chuan", {"chu", "an"}},
            {"chuai", {"chu", "ai"}},   {"biang", {"bi", "ang"}},
        };

    return innerSegment;
}

inline bool operator==(const PinyinEntry &a, const PinyinEntry &b) {
    return a.pinyin() == b.pinyin() && a.initial() == b.initial() &&
           a.final() == b.final() && a.flags() == b.flags();
}

inline bool operator!=(const PinyinEntry &a, const PinyinEntry &b) {
    return !(a == b);
}

const PinyinMap &getPinyinMap() {
    static const PinyinMap pinyinMap = {
        {"zuo", PinyinInitial::Z, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"zun", PinyinInitial::Z, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"zui", PinyinInitial::Z, PinyinFinal::UI, PinyinFuzzyFlag::None},
        {"zuagn",
         PinyinInitial::Z,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"zuang", PinyinInitial::Z, PinyinFinal::UAN,
         PinyinFuzzyFlag::UAN_UANG},
        {"zuagn",
         PinyinInitial::ZH,
         PinyinFinal::UANG,
         {PinyinFuzzyFlag::Z_ZH, PinyinFuzzyFlag::CommonTypo}},
        {"zuang", PinyinInitial::ZH, PinyinFinal::UANG, PinyinFuzzyFlag::Z_ZH},
        {"zuan", PinyinInitial::Z, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"zuai", PinyinInitial::ZH, PinyinFinal::UAI, PinyinFuzzyFlag::Z_ZH},
        {"zua", PinyinInitial::ZH, PinyinFinal::UA, PinyinFuzzyFlag::Z_ZH},
        {"zu", PinyinInitial::Z, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"zou", PinyinInitial::Z, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"zogn", PinyinInitial::Z, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"zon", PinyinInitial::Z, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"zong", PinyinInitial::Z, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"zi", PinyinInitial::Z, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"zhuo", PinyinInitial::ZH, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"zhun", PinyinInitial::ZH, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"zhui", PinyinInitial::ZH, PinyinFinal::UI, PinyinFuzzyFlag::None},
        {"zhuagn", PinyinInitial::ZH, PinyinFinal::UANG,
         PinyinFuzzyFlag::CommonTypo},
        {"zhuang", PinyinInitial::ZH, PinyinFinal::UANG, PinyinFuzzyFlag::None},
        {"zhuan", PinyinInitial::ZH, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"zhuai", PinyinInitial::ZH, PinyinFinal::UAI, PinyinFuzzyFlag::None},
        {"zhua", PinyinInitial::ZH, PinyinFinal::UA, PinyinFuzzyFlag::None},
        {"zhu", PinyinInitial::ZH, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"zhou", PinyinInitial::ZH, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"zhogn", PinyinInitial::ZH, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"zhon", PinyinInitial::ZH, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"zhong", PinyinInitial::ZH, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"zhi", PinyinInitial::ZH, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"zhegn", PinyinInitial::ZH, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"zheng", PinyinInitial::ZH, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"zhen", PinyinInitial::ZH, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"zhei", PinyinInitial::ZH, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"zhe", PinyinInitial::ZH, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"zhao", PinyinInitial::ZH, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"zhagn", PinyinInitial::ZH, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"zhang", PinyinInitial::ZH, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"zhan", PinyinInitial::ZH, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"zhai", PinyinInitial::ZH, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"zha", PinyinInitial::ZH, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"zegn", PinyinInitial::Z, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"zeng", PinyinInitial::Z, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"zen", PinyinInitial::Z, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"zei", PinyinInitial::Z, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"ze", PinyinInitial::Z, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"zao", PinyinInitial::Z, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"zagn", PinyinInitial::Z, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"zang", PinyinInitial::Z, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"zan", PinyinInitial::Z, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"zai", PinyinInitial::Z, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"za", PinyinInitial::Z, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"yun", PinyinInitial::Y, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"yue", PinyinInitial::Y, PinyinFinal::UE, PinyinFuzzyFlag::None},
        {"yve", PinyinInitial::Y, PinyinFinal::UE, PinyinFuzzyFlag::CommonTypo},
        {"yuagn",
         PinyinInitial::Y,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"yuang", PinyinInitial::Y, PinyinFinal::UAN,
         PinyinFuzzyFlag::UAN_UANG},
        {"yvagn",
         PinyinInitial::Y,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"yvang",
         PinyinInitial::Y,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"yuan", PinyinInitial::Y, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"yvan", PinyinInitial::Y, PinyinFinal::UAN,
         PinyinFuzzyFlag::CommonTypo},
        {"yu", PinyinInitial::Y, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"yv", PinyinInitial::Y, PinyinFinal::U, PinyinFuzzyFlag::CommonTypo},
        {"you", PinyinInitial::Y, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"yogn", PinyinInitial::Y, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"yon", PinyinInitial::Y, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"yong", PinyinInitial::Y, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"yo", PinyinInitial::Y, PinyinFinal::O, PinyinFuzzyFlag::None},
        {"yign", PinyinInitial::Y, PinyinFinal::ING,
         PinyinFuzzyFlag::CommonTypo},
        {"ying", PinyinInitial::Y, PinyinFinal::ING, PinyinFuzzyFlag::None},
        {"yin", PinyinInitial::Y, PinyinFinal::IN, PinyinFuzzyFlag::None},
        {"yi", PinyinInitial::Y, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"ye", PinyinInitial::Y, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"yao", PinyinInitial::Y, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"yagn", PinyinInitial::Y, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"yang", PinyinInitial::Y, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"yan", PinyinInitial::Y, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"ya", PinyinInitial::Y, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"xun", PinyinInitial::X, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"xue", PinyinInitial::X, PinyinFinal::UE, PinyinFuzzyFlag::None},
        {"xve", PinyinInitial::X, PinyinFinal::UE, PinyinFuzzyFlag::CommonTypo},
        {"xuagn",
         PinyinInitial::X,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"xuang", PinyinInitial::X, PinyinFinal::UAN,
         PinyinFuzzyFlag::UAN_UANG},
        {"xvagn",
         PinyinInitial::X,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"xvang",
         PinyinInitial::X,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"xuan", PinyinInitial::X, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"xvan", PinyinInitial::X, PinyinFinal::UAN,
         PinyinFuzzyFlag::CommonTypo},
        {"xu", PinyinInitial::X, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"xv", PinyinInitial::X, PinyinFinal::U, PinyinFuzzyFlag::CommonTypo},
        {"xou", PinyinInitial::X, PinyinFinal::U, PinyinFuzzyFlag::U_OU},
        {"xiu", PinyinInitial::X, PinyinFinal::IU, PinyinFuzzyFlag::None},
        {"xiogn", PinyinInitial::X, PinyinFinal::IONG,
         PinyinFuzzyFlag::CommonTypo},
        {"xion", PinyinInitial::X, PinyinFinal::IONG,
         PinyinFuzzyFlag::CommonTypo},
        {"xiong", PinyinInitial::X, PinyinFinal::IONG, PinyinFuzzyFlag::None},
        {"xign", PinyinInitial::X, PinyinFinal::ING,
         PinyinFuzzyFlag::CommonTypo},
        {"xing", PinyinInitial::X, PinyinFinal::ING, PinyinFuzzyFlag::None},
        {"xin", PinyinInitial::X, PinyinFinal::IN, PinyinFuzzyFlag::None},
        {"xie", PinyinInitial::X, PinyinFinal::IE, PinyinFuzzyFlag::None},
        {"xiao", PinyinInitial::X, PinyinFinal::IAO, PinyinFuzzyFlag::None},
        {"xiagn", PinyinInitial::X, PinyinFinal::IANG,
         PinyinFuzzyFlag::CommonTypo},
        {"xiang", PinyinInitial::X, PinyinFinal::IANG, PinyinFuzzyFlag::None},
        {"xian", PinyinInitial::X, PinyinFinal::IAN, PinyinFuzzyFlag::None},
        {"xia", PinyinInitial::X, PinyinFinal::IA, PinyinFuzzyFlag::None},
        {"xi", PinyinInitial::X, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"wu", PinyinInitial::W, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"wo", PinyinInitial::W, PinyinFinal::O, PinyinFuzzyFlag::None},
        {"wong", PinyinInitial::W, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"won", PinyinInitial::W, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"wogn", PinyinInitial::W, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"wegn", PinyinInitial::W, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"weng", PinyinInitial::W, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"wen", PinyinInitial::W, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"wei", PinyinInitial::W, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"wagn", PinyinInitial::W, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"wang", PinyinInitial::W, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"wan", PinyinInitial::W, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"wai", PinyinInitial::W, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"wa", PinyinInitial::W, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"tuo", PinyinInitial::T, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"tun", PinyinInitial::T, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"tui", PinyinInitial::T, PinyinFinal::UI, PinyinFuzzyFlag::None},
        {"tuagn",
         PinyinInitial::T,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"tuang", PinyinInitial::T, PinyinFinal::UAN,
         PinyinFuzzyFlag::UAN_UANG},
        {"tuan", PinyinInitial::T, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"tu", PinyinInitial::T, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"tou", PinyinInitial::T, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"togn", PinyinInitial::T, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"ton", PinyinInitial::T, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"tong", PinyinInitial::T, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"tign", PinyinInitial::T, PinyinFinal::ING,
         PinyinFuzzyFlag::CommonTypo},
        {"ting", PinyinInitial::T, PinyinFinal::ING, PinyinFuzzyFlag::None},
        {"tin", PinyinInitial::T, PinyinFinal::ING, PinyinFuzzyFlag::IN_ING},
        {"tie", PinyinInitial::T, PinyinFinal::IE, PinyinFuzzyFlag::None},
        {"tiao", PinyinInitial::T, PinyinFinal::IAO, PinyinFuzzyFlag::None},
        {"tiagn",
         PinyinInitial::T,
         PinyinFinal::IAN,
         {PinyinFuzzyFlag::IAN_IANG, PinyinFuzzyFlag::CommonTypo}},
        {"tiang", PinyinInitial::T, PinyinFinal::IAN,
         PinyinFuzzyFlag::IAN_IANG},
        {"tian", PinyinInitial::T, PinyinFinal::IAN, PinyinFuzzyFlag::None},
        {"ti", PinyinInitial::T, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"tegn", PinyinInitial::T, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"teng", PinyinInitial::T, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"ten", PinyinInitial::T, PinyinFinal::ENG, PinyinFuzzyFlag::EN_ENG},
        {"tei", PinyinInitial::T, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"te", PinyinInitial::T, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"tao", PinyinInitial::T, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"tagn", PinyinInitial::T, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"tang", PinyinInitial::T, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"tan", PinyinInitial::T, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"tai", PinyinInitial::T, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"ta", PinyinInitial::T, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"suo", PinyinInitial::S, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"sun", PinyinInitial::S, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"sui", PinyinInitial::S, PinyinFinal::UI, PinyinFuzzyFlag::None},
        {"suagn",
         PinyinInitial::SH,
         PinyinFinal::UANG,
         {PinyinFuzzyFlag::S_SH, PinyinFuzzyFlag::CommonTypo}},
        {"suang", PinyinInitial::SH, PinyinFinal::UANG, PinyinFuzzyFlag::S_SH},
        {"suagn",
         PinyinInitial::S,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"suang", PinyinInitial::S, PinyinFinal::UAN,
         PinyinFuzzyFlag::UAN_UANG},
        {"suan", PinyinInitial::S, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"suai", PinyinInitial::SH, PinyinFinal::UAI, PinyinFuzzyFlag::S_SH},
        {"sua", PinyinInitial::SH, PinyinFinal::UA, PinyinFuzzyFlag::S_SH},
        {"su", PinyinInitial::S, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"sou", PinyinInitial::S, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"sogn", PinyinInitial::S, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"son", PinyinInitial::S, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"song", PinyinInitial::S, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"si", PinyinInitial::S, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"shuo", PinyinInitial::SH, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"shun", PinyinInitial::SH, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"shui", PinyinInitial::SH, PinyinFinal::UI, PinyinFuzzyFlag::None},
        {"shuagn", PinyinInitial::SH, PinyinFinal::UANG,
         PinyinFuzzyFlag::CommonTypo},
        {"shuang", PinyinInitial::SH, PinyinFinal::UANG, PinyinFuzzyFlag::None},
        {"shuan", PinyinInitial::SH, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"shuai", PinyinInitial::SH, PinyinFinal::UAI, PinyinFuzzyFlag::None},
        {"shua", PinyinInitial::SH, PinyinFinal::UA, PinyinFuzzyFlag::None},
        {"shu", PinyinInitial::SH, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"shou", PinyinInitial::SH, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"shi", PinyinInitial::SH, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"shegn", PinyinInitial::SH, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"sheng", PinyinInitial::SH, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"shen", PinyinInitial::SH, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"shei", PinyinInitial::SH, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"she", PinyinInitial::SH, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"shao", PinyinInitial::SH, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"shagn", PinyinInitial::SH, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"shang", PinyinInitial::SH, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"shan", PinyinInitial::SH, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"shai", PinyinInitial::SH, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"sha", PinyinInitial::SH, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"segn", PinyinInitial::S, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"seng", PinyinInitial::S, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"sen", PinyinInitial::S, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"se", PinyinInitial::S, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"sao", PinyinInitial::S, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"sagn", PinyinInitial::S, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"sang", PinyinInitial::S, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"san", PinyinInitial::S, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"sai", PinyinInitial::S, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"sa", PinyinInitial::S, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"rua", PinyinInitial::R, PinyinFinal::UA, PinyinFuzzyFlag::None},
        {"r", PinyinInitial::R, PinyinFinal::Zero, PinyinFuzzyFlag::None},
        {"ruo", PinyinInitial::R, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"run", PinyinInitial::R, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"rui", PinyinInitial::R, PinyinFinal::UI, PinyinFuzzyFlag::None},
        {"ruagn",
         PinyinInitial::R,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"ruang", PinyinInitial::R, PinyinFinal::UAN,
         PinyinFuzzyFlag::UAN_UANG},
        {"ruan", PinyinInitial::R, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"ru", PinyinInitial::R, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"rou", PinyinInitial::R, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"rogn", PinyinInitial::R, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"ron", PinyinInitial::R, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"rong", PinyinInitial::R, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"ri", PinyinInitial::R, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"regn", PinyinInitial::R, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"reng", PinyinInitial::R, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"ren", PinyinInitial::R, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"re", PinyinInitial::R, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"rao", PinyinInitial::R, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"ragn", PinyinInitial::R, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"rang", PinyinInitial::R, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"ran", PinyinInitial::R, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"qun", PinyinInitial::Q, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"que", PinyinInitial::Q, PinyinFinal::UE, PinyinFuzzyFlag::None},
        {"qve", PinyinInitial::Q, PinyinFinal::UE, PinyinFuzzyFlag::CommonTypo},
        {"quagn",
         PinyinInitial::Q,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"quang", PinyinInitial::Q, PinyinFinal::UAN,
         PinyinFuzzyFlag::UAN_UANG},
        {"qvagn",
         PinyinInitial::Q,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"qvang",
         PinyinInitial::Q,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"quan", PinyinInitial::Q, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"qvan", PinyinInitial::Q, PinyinFinal::UAN,
         PinyinFuzzyFlag::CommonTypo},
        {"qu", PinyinInitial::Q, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"qv", PinyinInitial::Q, PinyinFinal::U, PinyinFuzzyFlag::CommonTypo},
        {"qiu", PinyinInitial::Q, PinyinFinal::IU, PinyinFuzzyFlag::None},
        {"qiogn", PinyinInitial::Q, PinyinFinal::IONG,
         PinyinFuzzyFlag::CommonTypo},
        {"qion", PinyinInitial::Q, PinyinFinal::IONG,
         PinyinFuzzyFlag::CommonTypo},
        {"qiong", PinyinInitial::Q, PinyinFinal::IONG, PinyinFuzzyFlag::None},
        {"qign", PinyinInitial::Q, PinyinFinal::ING,
         PinyinFuzzyFlag::CommonTypo},
        {"qing", PinyinInitial::Q, PinyinFinal::ING, PinyinFuzzyFlag::None},
        {"qin", PinyinInitial::Q, PinyinFinal::IN, PinyinFuzzyFlag::None},
        {"qie", PinyinInitial::Q, PinyinFinal::IE, PinyinFuzzyFlag::None},
        {"qiao", PinyinInitial::Q, PinyinFinal::IAO, PinyinFuzzyFlag::None},
        {"qiagn", PinyinInitial::Q, PinyinFinal::IANG,
         PinyinFuzzyFlag::CommonTypo},
        {"qiang", PinyinInitial::Q, PinyinFinal::IANG, PinyinFuzzyFlag::None},
        {"qian", PinyinInitial::Q, PinyinFinal::IAN, PinyinFuzzyFlag::None},
        {"qia", PinyinInitial::Q, PinyinFinal::IA, PinyinFuzzyFlag::None},
        {"qi", PinyinInitial::Q, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"pu", PinyinInitial::P, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"pou", PinyinInitial::P, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"po", PinyinInitial::P, PinyinFinal::O, PinyinFuzzyFlag::None},
        {"pign", PinyinInitial::P, PinyinFinal::ING,
         PinyinFuzzyFlag::CommonTypo},
        {"ping", PinyinInitial::P, PinyinFinal::ING, PinyinFuzzyFlag::None},
        {"pin", PinyinInitial::P, PinyinFinal::IN, PinyinFuzzyFlag::None},
        {"pie", PinyinInitial::P, PinyinFinal::IE, PinyinFuzzyFlag::None},
        {"piao", PinyinInitial::P, PinyinFinal::IAO, PinyinFuzzyFlag::None},
        {"piagn",
         PinyinInitial::P,
         PinyinFinal::IAN,
         {PinyinFuzzyFlag::IAN_IANG, PinyinFuzzyFlag::CommonTypo}},
        {"piang", PinyinInitial::P, PinyinFinal::IAN,
         PinyinFuzzyFlag::IAN_IANG},
        {"pian", PinyinInitial::P, PinyinFinal::IAN, PinyinFuzzyFlag::None},
        {"pi", PinyinInitial::P, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"pegn", PinyinInitial::P, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"peng", PinyinInitial::P, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"pen", PinyinInitial::P, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"pei", PinyinInitial::P, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"pao", PinyinInitial::P, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"pagn", PinyinInitial::P, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"pang", PinyinInitial::P, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"pan", PinyinInitial::P, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"pai", PinyinInitial::P, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"pa", PinyinInitial::P, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"ou", PinyinInitial::Zero, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"o", PinyinInitial::Zero, PinyinFinal::O, PinyinFuzzyFlag::None},
        {"nve", PinyinInitial::N, PinyinFinal::VE, PinyinFuzzyFlag::None},
        {"nv", PinyinInitial::N, PinyinFinal::V, PinyinFuzzyFlag::None},
        {"nuo", PinyinInitial::N, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"nun", PinyinInitial::N, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"nue", PinyinInitial::N, PinyinFinal::VE, PinyinFuzzyFlag::VE_UE},
        {"nuagn",
         PinyinInitial::N,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"nuang", PinyinInitial::N, PinyinFinal::UAN,
         PinyinFuzzyFlag::UAN_UANG},
        {"nuan", PinyinInitial::N, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"nu", PinyinInitial::N, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"nou", PinyinInitial::N, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"nogn", PinyinInitial::N, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"non", PinyinInitial::N, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"nong", PinyinInitial::N, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"niu", PinyinInitial::N, PinyinFinal::IU, PinyinFuzzyFlag::None},
        {"nign", PinyinInitial::N, PinyinFinal::ING,
         PinyinFuzzyFlag::CommonTypo},
        {"ning", PinyinInitial::N, PinyinFinal::ING, PinyinFuzzyFlag::None},
        {"nia", PinyinInitial::N, PinyinFinal::IA, PinyinFuzzyFlag::None},
        {"nin", PinyinInitial::N, PinyinFinal::IN, PinyinFuzzyFlag::None},
        {"nie", PinyinInitial::N, PinyinFinal::IE, PinyinFuzzyFlag::None},
        {"niao", PinyinInitial::N, PinyinFinal::IAO, PinyinFuzzyFlag::None},
        {"niagn", PinyinInitial::N, PinyinFinal::IANG,
         PinyinFuzzyFlag::CommonTypo},
        {"niang", PinyinInitial::N, PinyinFinal::IANG, PinyinFuzzyFlag::None},
        {"nian", PinyinInitial::N, PinyinFinal::IAN, PinyinFuzzyFlag::None},
        {"ni", PinyinInitial::N, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"ng", PinyinInitial::Zero, PinyinFinal::NG, PinyinFuzzyFlag::None},
        {"negn", PinyinInitial::N, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"neng", PinyinInitial::N, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"nen", PinyinInitial::N, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"nei", PinyinInitial::N, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"ne", PinyinInitial::N, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"nao", PinyinInitial::N, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"nagn", PinyinInitial::N, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"nang", PinyinInitial::N, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"nan", PinyinInitial::N, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"nai", PinyinInitial::N, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"na", PinyinInitial::N, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"n", PinyinInitial::N, PinyinFinal::Zero, PinyinFuzzyFlag::None},
        {"mu", PinyinInitial::M, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"mou", PinyinInitial::M, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"mo", PinyinInitial::M, PinyinFinal::O, PinyinFuzzyFlag::None},
        {"miu", PinyinInitial::M, PinyinFinal::IU, PinyinFuzzyFlag::None},
        {"mign", PinyinInitial::M, PinyinFinal::ING,
         PinyinFuzzyFlag::CommonTypo},
        {"ming", PinyinInitial::M, PinyinFinal::ING, PinyinFuzzyFlag::None},
        {"min", PinyinInitial::M, PinyinFinal::IN, PinyinFuzzyFlag::None},
        {"mie", PinyinInitial::M, PinyinFinal::IE, PinyinFuzzyFlag::None},
        {"miao", PinyinInitial::M, PinyinFinal::IAO, PinyinFuzzyFlag::None},
        {"miagn",
         PinyinInitial::M,
         PinyinFinal::IAN,
         {PinyinFuzzyFlag::IAN_IANG, PinyinFuzzyFlag::CommonTypo}},
        {"miang", PinyinInitial::M, PinyinFinal::IAN,
         PinyinFuzzyFlag::IAN_IANG},
        {"mian", PinyinInitial::M, PinyinFinal::IAN, PinyinFuzzyFlag::None},
        {"mi", PinyinInitial::M, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"megn", PinyinInitial::M, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"meng", PinyinInitial::M, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"men", PinyinInitial::M, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"mei", PinyinInitial::M, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"me", PinyinInitial::M, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"mao", PinyinInitial::M, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"magn", PinyinInitial::M, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"mang", PinyinInitial::M, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"man", PinyinInitial::M, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"mai", PinyinInitial::M, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"ma", PinyinInitial::M, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"m", PinyinInitial::M, PinyinFinal::Zero, PinyinFuzzyFlag::None},
        {"lve", PinyinInitial::L, PinyinFinal::VE, PinyinFuzzyFlag::None},
        {"lv", PinyinInitial::L, PinyinFinal::V, PinyinFuzzyFlag::None},
        {"luo", PinyinInitial::L, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"lun", PinyinInitial::L, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"lue", PinyinInitial::L, PinyinFinal::VE, PinyinFuzzyFlag::VE_UE},
        {"luagn",
         PinyinInitial::L,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"luang", PinyinInitial::L, PinyinFinal::UAN,
         PinyinFuzzyFlag::UAN_UANG},
        {"luan", PinyinInitial::L, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"lu", PinyinInitial::L, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"lou", PinyinInitial::L, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"logn", PinyinInitial::L, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"lon", PinyinInitial::L, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"long", PinyinInitial::L, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"lo", PinyinInitial::L, PinyinFinal::O, PinyinFuzzyFlag::None},
        {"liu", PinyinInitial::L, PinyinFinal::IU, PinyinFuzzyFlag::None},
        {"lign", PinyinInitial::L, PinyinFinal::ING,
         PinyinFuzzyFlag::CommonTypo},
        {"ling", PinyinInitial::L, PinyinFinal::ING, PinyinFuzzyFlag::None},
        {"lin", PinyinInitial::L, PinyinFinal::IN, PinyinFuzzyFlag::None},
        {"lie", PinyinInitial::L, PinyinFinal::IE, PinyinFuzzyFlag::None},
        {"liao", PinyinInitial::L, PinyinFinal::IAO, PinyinFuzzyFlag::None},
        {"liagn", PinyinInitial::L, PinyinFinal::IANG,
         PinyinFuzzyFlag::CommonTypo},
        {"liang", PinyinInitial::L, PinyinFinal::IANG, PinyinFuzzyFlag::None},
        {"lian", PinyinInitial::L, PinyinFinal::IAN, PinyinFuzzyFlag::None},
        {"lia", PinyinInitial::L, PinyinFinal::IA, PinyinFuzzyFlag::None},
        {"li", PinyinInitial::L, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"legn", PinyinInitial::L, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"leng", PinyinInitial::L, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"len", PinyinInitial::L, PinyinFinal::ENG, PinyinFuzzyFlag::EN_ENG},
        {"lei", PinyinInitial::L, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"le", PinyinInitial::L, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"lao", PinyinInitial::L, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"lagn", PinyinInitial::L, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"lang", PinyinInitial::L, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"lan", PinyinInitial::L, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"lai", PinyinInitial::L, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"la", PinyinInitial::L, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"kuo", PinyinInitial::K, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"kun", PinyinInitial::K, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"kui", PinyinInitial::K, PinyinFinal::UI, PinyinFuzzyFlag::None},
        {"kuagn", PinyinInitial::K, PinyinFinal::UANG,
         PinyinFuzzyFlag::CommonTypo},
        {"kuang", PinyinInitial::K, PinyinFinal::UANG, PinyinFuzzyFlag::None},
        {"kuan", PinyinInitial::K, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"kuai", PinyinInitial::K, PinyinFinal::UAI, PinyinFuzzyFlag::None},
        {"kua", PinyinInitial::K, PinyinFinal::UA, PinyinFuzzyFlag::None},
        {"ku", PinyinInitial::K, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"kou", PinyinInitial::K, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"kogn", PinyinInitial::K, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"kon", PinyinInitial::K, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"kong", PinyinInitial::K, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"kegn", PinyinInitial::K, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"keng", PinyinInitial::K, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"ken", PinyinInitial::K, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"kei", PinyinInitial::K, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"ke", PinyinInitial::K, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"kao", PinyinInitial::K, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"kagn", PinyinInitial::K, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"kang", PinyinInitial::K, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"kan", PinyinInitial::K, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"kai", PinyinInitial::K, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"ka", PinyinInitial::K, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"jun", PinyinInitial::J, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"jue", PinyinInitial::J, PinyinFinal::UE, PinyinFuzzyFlag::None},
        {"jve", PinyinInitial::J, PinyinFinal::UE, PinyinFuzzyFlag::CommonTypo},
        {"juagn",
         PinyinInitial::J,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"juang", PinyinInitial::J, PinyinFinal::UAN,
         PinyinFuzzyFlag::UAN_UANG},
        {"jvagn",
         PinyinInitial::J,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"jvang",
         PinyinInitial::J,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"juan", PinyinInitial::J, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"jvan", PinyinInitial::J, PinyinFinal::UAN,
         PinyinFuzzyFlag::CommonTypo},
        {"ju", PinyinInitial::J, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"jv", PinyinInitial::J, PinyinFinal::U, PinyinFuzzyFlag::CommonTypo},
        {"jiu", PinyinInitial::J, PinyinFinal::IU, PinyinFuzzyFlag::None},
        {"jiogn", PinyinInitial::J, PinyinFinal::IONG,
         PinyinFuzzyFlag::CommonTypo},
        {"jion", PinyinInitial::J, PinyinFinal::IONG,
         PinyinFuzzyFlag::CommonTypo},
        {"jiong", PinyinInitial::J, PinyinFinal::IONG, PinyinFuzzyFlag::None},
        {"jign", PinyinInitial::J, PinyinFinal::ING,
         PinyinFuzzyFlag::CommonTypo},
        {"jing", PinyinInitial::J, PinyinFinal::ING, PinyinFuzzyFlag::None},
        {"jin", PinyinInitial::J, PinyinFinal::IN, PinyinFuzzyFlag::None},
        {"jie", PinyinInitial::J, PinyinFinal::IE, PinyinFuzzyFlag::None},
        {"jiao", PinyinInitial::J, PinyinFinal::IAO, PinyinFuzzyFlag::None},
        {"jiagn", PinyinInitial::J, PinyinFinal::IANG,
         PinyinFuzzyFlag::CommonTypo},
        {"jiang", PinyinInitial::J, PinyinFinal::IANG, PinyinFuzzyFlag::None},
        {"jian", PinyinInitial::J, PinyinFinal::IAN, PinyinFuzzyFlag::None},
        {"jia", PinyinInitial::J, PinyinFinal::IA, PinyinFuzzyFlag::None},
        {"ji", PinyinInitial::J, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"huo", PinyinInitial::H, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"hun", PinyinInitial::H, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"hui", PinyinInitial::H, PinyinFinal::UI, PinyinFuzzyFlag::None},
        {"huagn", PinyinInitial::H, PinyinFinal::UANG,
         PinyinFuzzyFlag::CommonTypo},
        {"huang", PinyinInitial::H, PinyinFinal::UANG, PinyinFuzzyFlag::None},
        {"huan", PinyinInitial::H, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"huai", PinyinInitial::H, PinyinFinal::UAI, PinyinFuzzyFlag::None},
        {"hua", PinyinInitial::H, PinyinFinal::UA, PinyinFuzzyFlag::None},
        {"hu", PinyinInitial::H, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"hou", PinyinInitial::H, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"hogn", PinyinInitial::H, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"hon", PinyinInitial::H, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"hong", PinyinInitial::H, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"hegn", PinyinInitial::H, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"heng", PinyinInitial::H, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"hen", PinyinInitial::H, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"hei", PinyinInitial::H, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"he", PinyinInitial::H, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"hao", PinyinInitial::H, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"hagn", PinyinInitial::H, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"hang", PinyinInitial::H, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"han", PinyinInitial::H, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"hai", PinyinInitial::H, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"ha", PinyinInitial::H, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"guo", PinyinInitial::G, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"gun", PinyinInitial::G, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"gui", PinyinInitial::G, PinyinFinal::UI, PinyinFuzzyFlag::None},
        {"guagn", PinyinInitial::G, PinyinFinal::UANG,
         PinyinFuzzyFlag::CommonTypo},
        {"guang", PinyinInitial::G, PinyinFinal::UANG, PinyinFuzzyFlag::None},
        {"guan", PinyinInitial::G, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"guai", PinyinInitial::G, PinyinFinal::UAI, PinyinFuzzyFlag::None},
        {"gua", PinyinInitial::G, PinyinFinal::UA, PinyinFuzzyFlag::None},
        {"gu", PinyinInitial::G, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"gou", PinyinInitial::G, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"gogn", PinyinInitial::G, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"gon", PinyinInitial::G, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"gong", PinyinInitial::G, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"gegn", PinyinInitial::G, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"geng", PinyinInitial::G, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"gen", PinyinInitial::G, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"gei", PinyinInitial::G, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"ge", PinyinInitial::G, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"gao", PinyinInitial::G, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"gagn", PinyinInitial::G, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"gang", PinyinInitial::G, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"gan", PinyinInitial::G, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"gai", PinyinInitial::G, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"ga", PinyinInitial::G, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"fuai", PinyinInitial::H, PinyinFinal::UAI, PinyinFuzzyFlag::F_H},
        {"fu", PinyinInitial::F, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"fou", PinyinInitial::F, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"fo", PinyinInitial::F, PinyinFinal::O, PinyinFuzzyFlag::None},
        {"fiao", PinyinInitial::F, PinyinFinal::IAO, PinyinFuzzyFlag::None},
        {"fegn", PinyinInitial::F, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"feng", PinyinInitial::F, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"fen", PinyinInitial::F, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"fei", PinyinInitial::F, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"fagn", PinyinInitial::F, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"fang", PinyinInitial::F, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"fan", PinyinInitial::F, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"fa", PinyinInitial::F, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"er", PinyinInitial::Zero, PinyinFinal::ER, PinyinFuzzyFlag::None},
        {"egn", PinyinInitial::Zero, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"eng", PinyinInitial::Zero, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"en", PinyinInitial::Zero, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"ei", PinyinInitial::Zero, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"e", PinyinInitial::Zero, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"duo", PinyinInitial::D, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"dun", PinyinInitial::D, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"dui", PinyinInitial::D, PinyinFinal::UI, PinyinFuzzyFlag::None},
        {"duagn",
         PinyinInitial::D,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"duang", PinyinInitial::D, PinyinFinal::UAN,
         PinyinFuzzyFlag::UAN_UANG},
        {"duan", PinyinInitial::D, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"du", PinyinInitial::D, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"dou", PinyinInitial::D, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"dogn", PinyinInitial::D, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"don", PinyinInitial::D, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"dong", PinyinInitial::D, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"diu", PinyinInitial::D, PinyinFinal::IU, PinyinFuzzyFlag::None},
        {"dign", PinyinInitial::D, PinyinFinal::ING,
         PinyinFuzzyFlag::CommonTypo},
        {"ding", PinyinInitial::D, PinyinFinal::ING, PinyinFuzzyFlag::None},
        {"din", PinyinInitial::D, PinyinFinal::IN, PinyinFuzzyFlag::None},
        {"din", PinyinInitial::D, PinyinFinal::ING,
         PinyinFuzzyFlag::CommonTypo},
        {"die", PinyinInitial::D, PinyinFinal::IE, PinyinFuzzyFlag::None},
        {"diao", PinyinInitial::D, PinyinFinal::IAO, PinyinFuzzyFlag::None},
        {"diagn",
         PinyinInitial::D,
         PinyinFinal::IAN,
         {PinyinFuzzyFlag::IAN_IANG, PinyinFuzzyFlag::CommonTypo}},
        {"diang", PinyinInitial::D, PinyinFinal::IAN,
         PinyinFuzzyFlag::IAN_IANG},
        {"dian", PinyinInitial::D, PinyinFinal::IAN, PinyinFuzzyFlag::None},
        {"dia", PinyinInitial::D, PinyinFinal::IA, PinyinFuzzyFlag::None},
        {"di", PinyinInitial::D, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"degn", PinyinInitial::D, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"deng", PinyinInitial::D, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"den", PinyinInitial::D, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"dei", PinyinInitial::D, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"de", PinyinInitial::D, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"dao", PinyinInitial::D, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"dagn", PinyinInitial::D, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"dang", PinyinInitial::D, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"dan", PinyinInitial::D, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"dai", PinyinInitial::D, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"da", PinyinInitial::D, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"cuo", PinyinInitial::C, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"cun", PinyinInitial::C, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"cui", PinyinInitial::C, PinyinFinal::UI, PinyinFuzzyFlag::None},
        {"cuagn",
         PinyinInitial::C,
         PinyinFinal::UAN,
         {PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::CommonTypo}},
        {"cuang", PinyinInitial::C, PinyinFinal::UAN,
         PinyinFuzzyFlag::UAN_UANG},
        {"cuagn",
         PinyinInitial::CH,
         PinyinFinal::UANG,
         {PinyinFuzzyFlag::C_CH, PinyinFuzzyFlag::CommonTypo}},
        {"cuang", PinyinInitial::CH, PinyinFinal::UANG, PinyinFuzzyFlag::C_CH},
        {"cuan", PinyinInitial::C, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"cuai", PinyinInitial::CH, PinyinFinal::UAI, PinyinFuzzyFlag::C_CH},
        {"cu", PinyinInitial::C, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"cou", PinyinInitial::C, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"cogn", PinyinInitial::C, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"con", PinyinInitial::C, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"cong", PinyinInitial::C, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"ci", PinyinInitial::C, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"chuo", PinyinInitial::CH, PinyinFinal::UO, PinyinFuzzyFlag::None},
        {"chun", PinyinInitial::CH, PinyinFinal::UN, PinyinFuzzyFlag::None},
        {"chui", PinyinInitial::CH, PinyinFinal::UI, PinyinFuzzyFlag::None},
        {"chuagn", PinyinInitial::CH, PinyinFinal::UANG,
         PinyinFuzzyFlag::CommonTypo},
        {"chuang", PinyinInitial::CH, PinyinFinal::UANG, PinyinFuzzyFlag::None},
        {"chuan", PinyinInitial::CH, PinyinFinal::UAN, PinyinFuzzyFlag::None},
        {"chuai", PinyinInitial::CH, PinyinFinal::UAI, PinyinFuzzyFlag::None},
        {"chua", PinyinInitial::CH, PinyinFinal::UA, PinyinFuzzyFlag::None},
        {"chu", PinyinInitial::CH, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"chou", PinyinInitial::CH, PinyinFinal::OU, PinyinFuzzyFlag::None},
        {"chogn", PinyinInitial::CH, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"chon", PinyinInitial::CH, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"chong", PinyinInitial::CH, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"chi", PinyinInitial::CH, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"chegn", PinyinInitial::CH, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"cheng", PinyinInitial::CH, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"chen", PinyinInitial::CH, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"che", PinyinInitial::CH, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"chao", PinyinInitial::CH, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"chagn", PinyinInitial::CH, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"chang", PinyinInitial::CH, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"chan", PinyinInitial::CH, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"chai", PinyinInitial::CH, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"cha", PinyinInitial::CH, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"cegn", PinyinInitial::C, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"ceng", PinyinInitial::C, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"cen", PinyinInitial::C, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"ce", PinyinInitial::C, PinyinFinal::E, PinyinFuzzyFlag::None},
        {"cao", PinyinInitial::C, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"cagn", PinyinInitial::C, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"cang", PinyinInitial::C, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"can", PinyinInitial::C, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"cai", PinyinInitial::C, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"ca", PinyinInitial::C, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"bu", PinyinInitial::B, PinyinFinal::U, PinyinFuzzyFlag::None},
        {"bogn", PinyinInitial::B, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"bong", PinyinInitial::B, PinyinFinal::ONG, PinyinFuzzyFlag::None},
        {"bon", PinyinInitial::B, PinyinFinal::ONG,
         PinyinFuzzyFlag::CommonTypo},
        {"bo", PinyinInitial::B, PinyinFinal::O, PinyinFuzzyFlag::None},
        {"bign", PinyinInitial::B, PinyinFinal::ING,
         PinyinFuzzyFlag::CommonTypo},
        {"bing", PinyinInitial::B, PinyinFinal::ING, PinyinFuzzyFlag::None},
        {"bin", PinyinInitial::B, PinyinFinal::IN, PinyinFuzzyFlag::None},
        {"bie", PinyinInitial::B, PinyinFinal::IE, PinyinFuzzyFlag::None},
        {"biao", PinyinInitial::B, PinyinFinal::IAO, PinyinFuzzyFlag::None},
        {"biagn",
         PinyinInitial::B,
         PinyinFinal::IANG,
         {PinyinFuzzyFlag::CommonTypo}},
        {"biang", PinyinInitial::B, PinyinFinal::IANG, PinyinFuzzyFlag::None},
        {"bian", PinyinInitial::B, PinyinFinal::IAN, PinyinFuzzyFlag::None},
        {"bi", PinyinInitial::B, PinyinFinal::I, PinyinFuzzyFlag::None},
        {"begn", PinyinInitial::B, PinyinFinal::ENG,
         PinyinFuzzyFlag::CommonTypo},
        {"beng", PinyinInitial::B, PinyinFinal::ENG, PinyinFuzzyFlag::None},
        {"ben", PinyinInitial::B, PinyinFinal::EN, PinyinFuzzyFlag::None},
        {"bei", PinyinInitial::B, PinyinFinal::EI, PinyinFuzzyFlag::None},
        {"bao", PinyinInitial::B, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"bagn", PinyinInitial::B, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"bang", PinyinInitial::B, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"ban", PinyinInitial::B, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"bai", PinyinInitial::B, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"ba", PinyinInitial::B, PinyinFinal::A, PinyinFuzzyFlag::None},
        {"ao", PinyinInitial::Zero, PinyinFinal::AO, PinyinFuzzyFlag::None},
        {"agn", PinyinInitial::Zero, PinyinFinal::ANG,
         PinyinFuzzyFlag::CommonTypo},
        {"ang", PinyinInitial::Zero, PinyinFinal::ANG, PinyinFuzzyFlag::None},
        {"an", PinyinInitial::Zero, PinyinFinal::AN, PinyinFuzzyFlag::None},
        {"ai", PinyinInitial::Zero, PinyinFinal::AI, PinyinFuzzyFlag::None},
        {"a", PinyinInitial::Zero, PinyinFinal::A, PinyinFuzzyFlag::None},
    };
    return pinyinMap;
}

enum class FuzzyUpdatePhase {
    CommonTypo_UV_JQXY,
    CommonTypo_ON_ONG,
    CommonTypo_IN_ING,
    CommonTypo_Swap_NG_UE_UA_UAN,
    CommonTypo_Swap_UANG,
    AdvancedTypo_Swap_XH_UN,
    AdvancedTypo_Swap_Length2,
    AdvancedTypo_Swap_Length3,
    AdvancedTypo_Swap_Length4,
    AdvancedTypo_Swap_XHY_XYH,
};

PinyinFuzzyFlag fuzzyPhaseToFlag(FuzzyUpdatePhase phase) {
    switch (phase) {
    case FuzzyUpdatePhase::CommonTypo_UV_JQXY:
    case FuzzyUpdatePhase::CommonTypo_ON_ONG:
    case FuzzyUpdatePhase::CommonTypo_IN_ING:
    case FuzzyUpdatePhase::CommonTypo_Swap_NG_UE_UA_UAN:
    case FuzzyUpdatePhase::CommonTypo_Swap_UANG:
        return PinyinFuzzyFlag::CommonTypo;
    case FuzzyUpdatePhase::AdvancedTypo_Swap_XH_UN:
    case FuzzyUpdatePhase::AdvancedTypo_Swap_Length2:
    case FuzzyUpdatePhase::AdvancedTypo_Swap_Length3:
    case FuzzyUpdatePhase::AdvancedTypo_Swap_Length4:
    case FuzzyUpdatePhase::AdvancedTypo_Swap_XHY_XYH:
        return PinyinFuzzyFlag::AdvancedTypo;
    }
    return PinyinFuzzyFlag::CommonTypo;
}

std::optional<PinyinEntry> applyFuzzy(const PinyinEntry &entry,
                                      FuzzyUpdatePhase phase) {
    if (entry.pinyin() == "m" || entry.pinyin() == "n" ||
        entry.pinyin() == "r" || entry.pinyin() == "ng" ||
        entry.pinyin() == "ou") {
        return std::nullopt;
    }
    auto result = entry.pinyin();
    const PinyinFuzzyFlag fz = fuzzyPhaseToFlag(phase);
    switch (phase) {
    case FuzzyUpdatePhase::CommonTypo_UV_JQXY: {
        // Allow non standard usage like jv jve jvan jvuang
        if (result[0] == 'j' || result[0] == 'q' || result[0] == 'x' ||
            result[0] == 'y') {
            if (result.ends_with("u") && !result.ends_with("iu") &&
                !result.ends_with("ou")) {
                result.back() = 'v';
            }

            if (result.ends_with("ue") || result.ends_with("un")) {
                result[result.size() - 2] = 'v';
            }
            if (result.ends_with("uan")) {
                result[result.size() - 3] = 'v';
            }
            if (result.ends_with("uang")) {
                result[result.size() - 4] = 'v';
            }
        }
    } break;
    case FuzzyUpdatePhase::CommonTypo_ON_ONG:
        // Allow lon -> long
        if (result.ends_with("ong")) {
            result.pop_back();
        }
        break;
    case FuzzyUpdatePhase::CommonTypo_IN_ING:
        // Allow din -> ding
        if (result == "ding") {
            result.pop_back();
        }
        break;
    case FuzzyUpdatePhase::CommonTypo_Swap_NG_UE_UA_UAN:
        // Allow ying -> yign
        if (result.ends_with("ng")) {
            result[result.size() - 2] = 'g';
            result[result.size() - 1] = 'n';
        } else if (result.ends_with("ue")) {
            // Allow fuzzy for uv, that does not cause ambiguity.
            result[result.size() - 2] = 'e';
            result[result.size() - 1] = 'u';
        } else if (result.ends_with("ve")) {
            result[result.size() - 2] = 'e';
            result[result.size() - 1] = 'v';
        } else if (result.ends_with("ua")) {
            result[result.size() - 2] = 'a';
            result[result.size() - 1] = 'u';
        } else if (result.ends_with("uai") || result.ends_with("uan")) {
            result[result.size() - 3] = 'a';
            result[result.size() - 2] = 'u';
        } else if (result.ends_with("van")) {
            result[result.size() - 3] = 'a';
            result[result.size() - 2] = 'v';
        }
        break;
    case FuzzyUpdatePhase::CommonTypo_Swap_UANG:
        // this conflicts with "ng" rule, so need a separate pass.
        if (result.ends_with("uang")) {
            result[result.size() - 4] = 'a';
            result[result.size() - 3] = 'u';
        } else if (result.ends_with("vang")) {
            result[result.size() - 4] = 'a';
            result[result.size() - 3] = 'v';
        }
        break;
    case FuzzyUpdatePhase::AdvancedTypo_Swap_XH_UN:
        // Allow reversed zhe -> hze
        if (result.starts_with("zh") || result.starts_with("sh") ||
            result.starts_with("ch")) {
            std::swap(result[0], result[1]);
        } else if (result.ends_with("un") && !result.ends_with("aun")) {
            result[result.size() - 2] = 'n';
            result[result.size() - 1] = 'u';
        }
        break;
    case FuzzyUpdatePhase::AdvancedTypo_Swap_Length2:
        if (entry.flags().test(PinyinFuzzyFlag::AdvancedTypo)) {
            break;
        }
        for (const auto *const two : {"ai", "ia", "ei", "ie", "ao", "uo", "ou",
                                      "iu", "an", "en", "in"}) {
            if (result.ends_with(two)) {
                std::swap(result[result.size() - 2], result[result.size() - 1]);
            }
        }
        break;
    case FuzzyUpdatePhase::AdvancedTypo_Swap_Length3:
        if (entry.flags().test(PinyinFuzzyFlag::AdvancedTypo)) {
            break;
        }
        for (const auto *const three :
             {"ang", "eng", "ing", "ong", "iao", "ian"}) {
            if (result.ends_with(three)) {
                std::swap(result[result.size() - 3], result[result.size() - 2]);
            }
        }
        break;

    case FuzzyUpdatePhase::AdvancedTypo_Swap_Length4:
        if (entry.flags().test(PinyinFuzzyFlag::AdvancedTypo)) {
            break;
        }
        for (const auto *const four : {"iang", "iong"}) {
            if (result.ends_with(four)) {
                std::swap(result[result.size() - 4], result[result.size() - 3]);
            }
        }
        break;

    case FuzzyUpdatePhase::AdvancedTypo_Swap_XHY_XYH:
        if (entry.flags().test(PinyinFuzzyFlag::AdvancedTypo)) {
            break;
        }
        // zhe -> zeh.
        if (result.size() == 3 && result[1] == 'h' &&
            entry.flags() == PinyinFuzzyFlag::None) {
            std::swap(result[result.size() - 2], result[result.size() - 1]);
        }
        break;
    default:
        break;
    }
    if (result == entry.pinyin()) {
        return std::nullopt;
    }
    return PinyinEntry(result.data(), entry.initial(), entry.final(),
                       entry.flags() | fz);
}

std::optional<PinyinEntry> applyFuzzy(const PinyinEntry &entry,
                                      PinyinFuzzyFlag fz) {
    auto result = entry.pinyin();
    switch (fz) {
    case PinyinFuzzyFlag::VE_UE: {
        if (result.ends_with("ve")) {
            result[result.size() - 2] = 'u';
        }
        break;
    }

    case PinyinFuzzyFlag::IAN_IANG: {
        if (result.ends_with("ian")) {
            result.push_back('g');
        } else if (result.ends_with("iang")) {
            result.pop_back();
        }
        break;
    }

    case PinyinFuzzyFlag::UAN_UANG: {
        if (entry.flags() != PinyinFuzzyFlag::None) {
            break;
        }
        if (result.ends_with("uan")) {
            result.push_back('g');
        } else if (result.ends_with("uang")) {
            result.pop_back();
        }
        break;
    }

    case PinyinFuzzyFlag::AN_ANG: {
        if (result.ends_with("uan") || result.ends_with("uang")) {
            break;
        }
        if (result.ends_with("ian") || result.ends_with("iang")) {
            break;
        }
        if (result.ends_with("an")) {
            result.push_back('g');
        } else if (result.ends_with("ang")) {
            result.pop_back();
        }
        break;
    }

    case PinyinFuzzyFlag::EN_ENG: {
        if (result.ends_with("en")) {
            result.push_back('g');
        } else if (result.ends_with("eng")) {
            result.pop_back();
        }
        break;
    }

    case PinyinFuzzyFlag::IN_ING: {
        if (result.ends_with("in")) {
            result.push_back('g');
        } else if (result.ends_with("ing")) {
            result.pop_back();
        }
        break;
    }

    case PinyinFuzzyFlag::U_OU: {
        if (result.ends_with("ou")) {
            result.pop_back();
            result.back() = 'u';
        } else if (result.ends_with("u") && !result.ends_with("iu")) {
            result.back() = 'o';
            result.push_back('u');
        }
        break;
    }

    case PinyinFuzzyFlag::C_CH: {
        if (entry.flags() != PinyinFuzzyFlag::None) {
            break;
        }
        if (result.starts_with("ch")) {
            result.erase(std::next(result.begin()));
        } else if (result.starts_with("c")) {
            result.insert(std::next(result.begin()), 'h');
        }
        break;
    }

    case PinyinFuzzyFlag::S_SH: {
        if (entry.flags() != PinyinFuzzyFlag::None) {
            break;
        }
        if (result.starts_with("sh")) {
            result.erase(std::next(result.begin()));
        } else if (result.starts_with("s")) {
            result.insert(std::next(result.begin()), 'h');
        }
        break;
    }

    case PinyinFuzzyFlag::Z_ZH: {
        if (entry.flags() != PinyinFuzzyFlag::None) {
            break;
        }
        if (result.starts_with("zh")) {
            result.erase(std::next(result.begin()));
        } else if (result.starts_with("z")) {
            result.insert(std::next(result.begin()), 'h');
        }
        break;
    }

    case PinyinFuzzyFlag::F_H: {
        if (result.starts_with("f")) {
            result.front() = 'h';
        } else if (result.starts_with("h")) {
            result.front() = 'f';
        }
        break;
    }

    case PinyinFuzzyFlag::L_N: {
        if (result.starts_with("l")) {
            result.front() = 'n';
        } else if (result.starts_with("n")) {
            result.front() = 'l';
        }
        break;
    }

    case PinyinFuzzyFlag::L_R: {
        if (result.starts_with("l")) {
            result.front() = 'r';
        } else if (result.starts_with("r")) {
            result.front() = 'l';
        }
        break;
    }
    default:
        break;
    }
    if (result == entry.pinyin()) {
        return std::nullopt;
    }
    return PinyinEntry(result.data(), entry.initial(), entry.final(),
                       entry.flags() | fz);
}

template <typename T>
void applyFuzzyToMap(PinyinMap &map, T fuzzy) {
    std::vector<PinyinEntry> newEntries;
    for (const auto &entry : map) {
        if (auto newEntry = applyFuzzy(entry, fuzzy)) {
            newEntries.push_back(*newEntry);
        }
    }

    for (const auto &newEntry : newEntries) {
        if (auto iter = map.find(newEntry.pinyin()); iter != map.end()) {
            bool force = false;
            if constexpr (std::is_same_v<T, FuzzyUpdatePhase>) {
                if (fuzzy == FuzzyUpdatePhase::CommonTypo_IN_ING) {
                    force = true;
                }
            }
            if (!force && iter->flags() == PinyinFuzzyFlag::None) {
                continue;
            }
        }
        FCITX_ASSERT(map.insert(newEntry).second);
    }
}

const PinyinMap &getPinyinMapV2() {
    static const PinyinMap map = []() {
        const PinyinMap &orig = getPinyinMap();

        PinyinMap filtered;
        for (const auto &entry : orig) {
            if (entry.flags() == PinyinFuzzyFlag::None) {
                filtered.insert(entry);
            }
        }

        for (auto fz : {PinyinFuzzyFlag::U_OU, PinyinFuzzyFlag::IN_ING,
                        PinyinFuzzyFlag::EN_ENG, PinyinFuzzyFlag::AN_ANG,
                        PinyinFuzzyFlag::UAN_UANG, PinyinFuzzyFlag::IAN_IANG,
                        PinyinFuzzyFlag::VE_UE, PinyinFuzzyFlag::F_H,
                        PinyinFuzzyFlag::L_N, PinyinFuzzyFlag::Z_ZH,
                        PinyinFuzzyFlag::S_SH, PinyinFuzzyFlag::C_CH,
                        PinyinFuzzyFlag::L_R}) {
            applyFuzzyToMap(filtered, fz);
        }

        for (auto phase : {
                 FuzzyUpdatePhase::CommonTypo_UV_JQXY,
                 FuzzyUpdatePhase::CommonTypo_ON_ONG,
                 FuzzyUpdatePhase::CommonTypo_IN_ING,
                 FuzzyUpdatePhase::CommonTypo_Swap_NG_UE_UA_UAN,
                 FuzzyUpdatePhase::CommonTypo_Swap_UANG,
                 FuzzyUpdatePhase::AdvancedTypo_Swap_XH_UN,
                 FuzzyUpdatePhase::AdvancedTypo_Swap_Length2,
                 FuzzyUpdatePhase::AdvancedTypo_Swap_Length3,
                 FuzzyUpdatePhase::AdvancedTypo_Swap_Length4,
                 FuzzyUpdatePhase::AdvancedTypo_Swap_XHY_XYH,
             }) {
            applyFuzzyToMap(filtered, phase);
        }
        return filtered;
    }();
    return map;
}

} // namespace libime
