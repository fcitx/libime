/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <fcntl.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <fcitx-utils/fdstreambuf.h>
#include <fcitx-utils/standardpaths.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/unixfd.h>
#include "libime/core/historybigram.h"
#include "libime/core/utils_p.h"
#include "libime/pinyin/pinyindictionary.h"

static const std::array<std::string, 412> PYFA = {
    "AA", "AB", "AC", "AD", "AE", "AF", "AH", "AI", "AJ", "AU", "AV", "AW",
    "AX", "AY", "AZ", "Aa", "Ac", "Ad", "Ae", "BA", "BB", "BC", "BD", "BE",
    "BF", "BG", "BH", "BI", "BJ", "BU", "BV", "BW", "BZ", "Bc", "Bd", "Be",
    "CA", "CC", "CD", "CE", "CF", "CJ", "CP", "CQ", "CT", "CU", "CV", "CW",
    "CZ", "Cb", "Cd", "DJ", "DK", "DL", "DM", "DN", "DO", "DP", "DQ", "DR",
    "DS", "DW", "DZ", "Db", "Dd", "EA", "EB", "EC", "ED", "EG", "EH", "EI",
    "ET", "EW", "FA", "FB", "FC", "FD", "FE", "FF", "FG", "FI", "FJ", "FL",
    "FN", "FO", "FQ", "FU", "FV", "FW", "FZ", "Fc", "Fd", "Fe", "GA", "GB",
    "GC", "GD", "GE", "GF", "GG", "GH", "GI", "GJ", "GV", "GW", "GX", "GY",
    "GZ", "Ga", "Gc", "Gd", "Ge", "HA", "HB", "HC", "HD", "HE", "HF", "HH",
    "HI", "HJ", "HU", "HV", "HW", "HZ", "Hc", "Hd", "He", "IC", "ID", "IE",
    "IF", "IH", "II", "IJ", "IU", "IV", "IW", "IZ", "Ic", "Id", "Ie", "JJ",
    "JK", "JL", "JM", "JN", "JO", "JP", "JQ", "JR", "JS", "JW", "JZ", "Jb",
    "Jd", "KA", "KB", "KC", "KD", "KE", "KG", "KH", "KI", "KJ", "KL", "KN",
    "KO", "KP", "KQ", "KT", "KV", "KW", "L ", "M ", "N ", "O ", "OA", "OB",
    "OC", "OD", "OE", "OF", "OG", "OH", "OI", "OJ", "OL", "OM", "ON", "OO",
    "OP", "OQ", "OS", "OU", "OV", "OW", "OZ", "Ob", "Oe", "Of", "Og", "P ",
    "PA", "PB", "PC", "PD", "PE", "PF", "PG", "PH", "PI", "PJ", "PL", "PN",
    "PO", "PP", "PQ", "PS", "PT", "PV", "PW", "QA", "QB", "QC", "QD", "QE",
    "QF", "QG", "QI", "QJ", "QK", "QL", "QM", "QN", "QO", "QP", "QQ", "QS",
    "QT", "QU", "QV", "QW", "QZ", "Qb", "Qd", "Qe", "Qf", "Qg", "RA", "RB",
    "RC", "RD", "RE", "RF", "RG", "RH", "RI", "RU", "RV", "RW", "RX", "RY",
    "RZ", "Ra", "Rc", "Rd", "Re", "SJ", "SK", "SL", "SM", "SN", "SO", "SP",
    "SQ", "SR", "SS", "SW", "SZ", "Sb", "Sd", "TA", "TB", "TC", "TD", "TE",
    "TF", "TG", "TH", "TI", "TU", "TV", "TW", "TX", "TY", "TZ", "Ta", "Tc",
    "Td", "Te", "UA", "UB", "UC", "UD", "UE", "UF", "UG", "UH", "UI", "UU",
    "UV", "UW", "UX", "UY", "UZ", "Ua", "Uc", "Ud", "Ue", "VA", "VC", "VD",
    "VG", "VH", "VI", "VT", "VV", "VW", "W ", "X ", "Y ", "Z ", "aA", "aB",
    "aC", "aD", "aE", "aF", "aG", "aH", "aI", "aJ", "aK", "aL", "aN", "aO",
    "aQ", "aS", "aU", "aV", "aW", "aZ", "ac", "ad", "ae", "bA", "bB", "bC",
    "bD", "bE", "bF", "bH", "bI", "bJ", "bU", "bV", "bW", "bY", "bZ", "ba",
    "bc", "bd", "be", "cA", "cB", "cC", "cD", "cE", "cF", "cH", "cI", "cJ",
    "cU", "cV", "cW", "cZ", "cc", "cd", "ce", "dA", "dB", "dC", "dD", "dE",
    "dG", "dH", "dI", "dJ", "dL", "dN", "dO", "dP", "dQ", "dT", "dW", "e ",
    "f ", "g ", "h ", "i ",
};

static const std::unordered_map<char, std::string> consonantMapTable = {
    {'A', "a"},   {'B', "ai"},   {'C', "an"},   {'D', "ang"}, {'E', "ao"},
    {'F', "e"},   {'G', "ei"},   {'H', "en"},   {'I', "eng"}, {'J', "i"},
    {'K', "ia"},  {'L', "ian"},  {'M', "iang"}, {'N', "iao"}, {'O', "ie"},
    {'P', "in"},  {'Q', "ing"},  {'R', "iong"}, {'S', "iu"},  {'T', "o"},
    {'U', "ong"}, {'V', "ou"},   {'W', "u"},    {'X', "ua"},  {'Y', "uai"},
    {'Z', "uan"}, {'a', "uang"}, {'b', "ue"},   {'c', "ui"},  {'d', "un"},
    {'e', "uo"},  {'f', "v"},    {'g', "ve"},   {' ', ""},
};

static const std::unordered_map<char, std::string> syllabaryMapTable = {
    {'A', "zh"}, {'B', "z"},   {'C', "y"},  {'D', "x"},  {'E', "w"},
    {'F', "t"},  {'G', "sh"},  {'H', "s"},  {'I', "r"},  {'J', "q"},
    {'K', "p"},  {'L', "ou"},  {'M', "o"},  {'N', "ng"}, {'O', "n"},
    {'P', "m"},  {'Q', "l"},   {'R', "k"},  {'S', "j"},  {'T', "h"},
    {'U', "g"},  {'V', "f"},   {'W', "er"}, {'X', "en"}, {'Y', "ei"},
    {'Z', "e"},  {'a', "d"},   {'b', "ch"}, {'c', "c"},  {'d', "b"},
    {'e', "ao"}, {'f', "ang"}, {'g', "an"}, {'h', "ai"}, {'i', "a"},
};

using namespace libime;
using namespace fcitx;

struct UserPhrase {
    std::vector<char> Map;
    std::string Phrase;
    uint32_t Index;
    uint32_t freq;
};

struct PYMB {
    uint32_t PYFAIndex;
    std::string HZ;

    std::vector<UserPhrase> userPhrase;
};

auto LoadPYMB(std::istream &in) {
    std::vector<PYMB> PYMB;

    std::vector<char> buffer;
    while (true) {
        uint32_t PYFAIndex;
        if (!unmarshallLE(in, PYFAIndex)) {
            break;
        }
        PYMB.emplace_back();
        auto &item = PYMB.back();
        item.PYFAIndex = PYFAIndex;

        int8_t clen;
        throw_if_io_fail(unmarshall(in, clen));
        buffer.resize(clen);
        throw_if_io_fail(unmarshallVector(in, buffer));
        buffer.push_back('\0');
        item.HZ = buffer.data();
        uint32_t userPhraseCount;
        throw_if_io_fail(unmarshallLE(in, userPhraseCount));
        item.userPhrase.reserve(userPhraseCount);

        for (uint32_t j = 0; j < userPhraseCount; ++j) {
            item.userPhrase.emplace_back();
            uint32_t length;
            throw_if_io_fail(unmarshallLE(in, length));
            item.userPhrase[j].Map.resize(length);
            throw_if_io_fail(unmarshallVector(in, item.userPhrase[j].Map));

            throw_if_io_fail(unmarshallLE(in, length));
            buffer.resize(length);
            throw_if_io_fail(unmarshallVector(in, buffer));
            buffer.push_back('\0');
            item.userPhrase[j].Phrase = buffer.data();
            throw_if_io_fail(unmarshallLE(in, item.userPhrase[j].Index));
            throw_if_io_fail(unmarshallLE(in, item.userPhrase[j].freq));
        }
    }

    return PYMB;
}

template <typename T>
std::string decodeFcitx4Pinyin(const T &code) {
    assert(code.size() % 2 == 0);
    std::vector<std::string> pys;
    for (size_t i = 0; i < code.size() / 2; i++) {
        std::string py;
        if (auto iter = syllabaryMapTable.find(code[2 * i]);
            iter != syllabaryMapTable.end()) {
            py.append(iter->second);
        } else {
            return {};
        }
        if (auto iter = consonantMapTable.find(code[(2 * i) + 1]);
            iter != consonantMapTable.end()) {
            py.append(iter->second);
        } else {
            return {};
        }
        pys.push_back(py);
    }

    return stringutils::join(pys, '\'');
}

static std::string_view argv0;

void usage(const char *extra = nullptr) {
    std::cout
        << "Usage: " << argv0
        << " [-o <dict>/-O] [-p <history>/-P] [-U]"
           "[source]"
        << '\n'
        << "source: the source file of the dictionary." << '\n'
        << "        If it's empty, default fcitx 4 user file will be used."
        << '\n'
        << "-o: output dict file path" << '\n'
        << "-O: Skip dict file." << '\n'
        << "-p: history file path" << '\n'
        << "-P: Skip history file" << '\n'
        << "-U: overwrite instead of merge with existing data." << '\n'
        << "-h: Show this help" << '\n';
    if (extra) {
        std::cout << extra << '\n';
    }
}

int main(int argc, char *argv[]) {
    argv0 = argv[0];
    bool skipDict = false;
    bool skipHistory = false;
    const char *dictFile = nullptr;
    const char *historyFile = nullptr;
    bool merge = true;
    int c;
    while ((c = getopt(argc, argv, "Oo:Pp:UXh")) != -1) {
        switch (c) {
        case 'O':
            skipDict = true;
            break;
        case 'o':
            dictFile = optarg;
            break;
        case 'P':
            skipHistory = true;
            break;
        case 'p':
            historyFile = optarg;
            break;
        case 'U':
            merge = false;
            break;
        case 'h':
            usage();
            return 0;
        default:
            usage();
            return 1;
        }
    }

    UnixFD sourceFd;
    if (optind < argc) {
        sourceFd = UnixFD::own(open(argv[optind], O_RDONLY));
    } else {
        // Fcitx 4's xdg is not following the spec, it's using a xdg_data and
        // xdg_config in mixed way.
        sourceFd = StandardPaths::global().open(StandardPathsType::Config,
                                                "fcitx/pinyin/pyusrphrase.mb",
                                                StandardPathsMode::User);
    }
    if (!sourceFd.isValid()) {
        std::cout << "Failed to open the source file." << '\n';
    }

    std::vector<PYMB> pymbs;
    try {
        IFDStreamBuf buffer(sourceFd.fd());
        std::istream in(&buffer);
        pymbs = LoadPYMB(in);
    } catch (const std::exception &e) {
        std::cout << "Failed when loading source file: " << e.what();
        return 1;
    }

    PinyinDictionary dict;
    UnixFD dictFD;
    if (!skipDict && merge) {
        if (dictFile) {
            dictFD = UnixFD::own(open(dictFile, O_RDONLY));
        } else {
            dictFD = StandardPaths::global().open(StandardPathsType::PkgData,
                                                  "pinyin/user.dict",
                                                  StandardPathsMode::User);
        }
    }

    if (dictFD.isValid()) {
        try {
            IFDStreamBuf buffer(dictFD.fd());
            std::istream in(&buffer);
            dict.load(PinyinDictionary::SystemDict, in,
                      PinyinDictFormat::Binary);
        } catch (const std::exception &e) {
            std::cout << "Failed to load dict file: " << e.what();
            return 1;
        }
    }

    HistoryBigram history;
    UnixFD historyFD;
    if (!skipHistory && merge) {
        if (historyFile) {
            historyFD = UnixFD::own(open(historyFile, O_RDONLY));
        } else {
            historyFD = StandardPaths::global().open(StandardPathsType::PkgData,
                                                     "pinyin/user.history",
                                                     StandardPathsMode::User);
        }
    }

    if (historyFD.isValid()) {
        try {
            IFDStreamBuf buffer(historyFD.fd());
            std::istream in(&buffer);
            history.load(in);
        } catch (const std::exception &e) {
            std::cout << "Failed to load history file: " << e.what();
            return 1;
        }
    }

    for (const auto &pymb : pymbs) {
        if (pymb.PYFAIndex >= PYFA.size()) {
            continue;
        }
        auto firstPy = decodeFcitx4Pinyin(PYFA[pymb.PYFAIndex]);
        if (firstPy.empty()) {
            continue;
        }

        for (const auto &userPhrase : pymb.userPhrase) {
            auto restPy = decodeFcitx4Pinyin(userPhrase.Map);
            if (restPy.empty()) {
                continue;
            }
            try {
                auto word = stringutils::concat(pymb.HZ, userPhrase.Phrase);
                if (!skipDict) {
                    dict.addWord(PinyinDictionary::SystemDict,
                                 stringutils::concat(firstPy, "\'", restPy),
                                 word);
                }
                if (!skipHistory) {
                    for (uint32_t i = 0;
                         i <
                         std::min(static_cast<uint32_t>(10U), userPhrase.freq);
                         i++) {
                        history.add({word});
                    }
                }
            } catch (...) {
            }
        }
    }

    if (!skipDict) {
        std::filesystem::path outputDictFile = "pinyin/user.dict";
        if (dictFile) {
            if (dictFile[0] == '/') {
                outputDictFile = dictFile;
            } else {
                outputDictFile = std::filesystem::absolute(dictFile);
            }
        }
        StandardPaths::global().safeSave(
            StandardPathsType::PkgData, outputDictFile, [&dict](int fd) {
                OFDStreamBuf buffer(fd);
                std::ostream out(&buffer);
                dict.save(PinyinDictionary::SystemDict, out,
                          PinyinDictFormat::Binary);
                return true;
            });
    }

    if (!skipHistory) {
        std::filesystem::path outputHistoryFile = "pinyin/user.history";
        if (historyFile) {
            if (historyFile[0] == '/') {
                outputHistoryFile = historyFile;
            } else {
                outputHistoryFile = std::filesystem::absolute(historyFile);
            }
        }
        StandardPaths::global().safeSave(StandardPathsType::PkgData,
                                         outputHistoryFile, [&history](int fd) {
                                             OFDStreamBuf buffer(fd);
                                             std::ostream out(&buffer);
                                             history.save(out);
                                             return true;
                                         });
    }

    return 0;
}
