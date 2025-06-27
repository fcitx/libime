/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/stringutils.h>
#include "libime/core/constants.h"
#include "libime/core/datrie.h"
#include "libime/core/languagemodel.h"

void usage(const char *argv0) {
    std::cout
        << "Usage: " << argv0
        << " [-f <score>] [-s <maxSize>] <model.lm> <source.arpa> <dest>\n"
        << " -d <source.lm.predict> <dest>\n"
        << " -f: Set score filter\n"
        << " -s: Set max number of prediction per word\n"
        << " -h: Show this help\n";
}

float score(const libime::LanguageModel &model, std::string_view w,
            std::string_view w2) {
    return model.wordsScore(model.nullState(),
                            std::vector<std::string_view>{w, w2});
}

void parse(const char *modelFile, const char *arpa, const char *output,
           float filter, unsigned long maxSize) {
    using namespace libime;
    LanguageModel model(modelFile);
    DATrie<float> trie;

    std::ifstream fin;
    std::istream *in;
    if (std::string_view(arpa) == "-") {
        in = &std::cin;
    } else {
        fin.open(arpa, std::ios::in | std::ios::binary);
        in = &fin;
    }

    std::string lineBuf;
    std::unordered_map<std::string, std::unordered_map<std::string, float>>
        word;
    int grams = -1;
    while (!in->eof()) {
        if (!std::getline(*in, lineBuf)) {
            break;
        }

        auto line = fcitx::stringutils::trimView(lineBuf);

        if (line == "\\2-grams:") {
            grams = 2;
            continue;
        }

        if (line == "\\3-grams:") {
            break;
        }

        if (grams < 0) {
            continue;
        }
        std::vector<std::string> tokens =
            fcitx::stringutils::split(line, FCITX_WHITESPACE);

        if (tokens.size() < static_cast<size_t>(grams) + 1) {
            continue;
        }
        // We don't want prediction generate <unk>
        if (std::find(tokens.begin() + 1, tokens.end(), "<unk>") !=
            tokens.end()) {
            continue;
        }

        auto s = score(model, tokens[grams - 1], tokens[grams]);
        if (s > filter) {
            word[tokens[grams - 1]][tokens[grams]] = s;
        }
    }

    for (auto &p : word) {
        std::vector<std::pair<std::string, float>> result(p.second.begin(),
                                                          p.second.end());
        std::ranges::sort(result, [](const auto &lhs, const auto &rhs) {
            if (lhs.second != rhs.second) {
                return lhs.second > rhs.second;
            }
            return lhs.first < rhs.first;
        });
        if (result.size() > maxSize) {
            result.resize(maxSize);
        }
        for (auto &p2 : result) {
            trie.set(p.first + "|" + p2.first, p2.second);
        }
    }

    FCITX_LOG(Info) << "Memory: " << trie.mem_size()
                    << " Number of entries: " << trie.size();

    std::ofstream fout;
    std::ostream *out;
    if (std::string_view(output) == "-") {
        out = &std::cout;
    } else {
        fout.open(output, std::ios::out | std::ios::binary);
        out = &fout;
    }
    trie.save(*out);
}

void dump(const char *input, const char *output) {
    using namespace libime;
    std::ifstream fin;
    std::istream *in;
    if (std::string_view(input) == "-") {
        in = &std::cin;
    } else {
        fin.open(input, std::ios::in | std::ios::binary);
        in = &fin;
    }

    DATrie<float> trie;
    trie.load(*in);

    std::ofstream fout;
    std::ostream *out;
    if (std::string_view(output) == "-") {
        out = &std::cout;
    } else {
        fout.open(output, std::ios::out | std::ios::binary);
        out = &fout;
    }
    trie.foreach([out, &trie](float value, size_t len, uint64_t pos) {
        std::string s;
        trie.suffix(s, len, pos);
        *out << s << " " << value << '\n';
        return true;
    });
}

int main(int argc, char *argv[]) {
    int c;
    unsigned long maxSize = 15;
    float filter =
        std::log10(libime::DEFAULT_LANGUAGE_MODEL_UNKNOWN_PROBABILITY_PENALTY) +
        1;
    bool doDump = false;
    while ((c = getopt(argc, argv, "df:s:h")) != -1) {
        switch (c) {
        case 'd':
            doDump = true;
            break;
        case 'f':
            filter = std::stof(optarg);
            break;
        case 's':
            maxSize = std::stoul(optarg);
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (doDump) {
        if (optind + 1 >= argc) {
            usage(argv[0]);
            return 1;
        }
        dump(argv[optind], argv[optind + 1]);
    } else {
        if (optind + 2 >= argc) {
            usage(argv[0]);
            return 1;
        }
        parse(argv[optind], argv[optind + 1], argv[optind + 2], filter,
              maxSize);
    }
    return 0;
}
