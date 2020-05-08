/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/constants.h"
#include "libime/core/datrie.h"
#include "libime/core/languagemodel.h"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <fcitx-utils/log.h>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

void usage(const char *argv0) {
    std::cout << "Usage: " << argv0
              << " [-f <score>] [-s <maxSize>] <source> <dest>" << std::endl
              << "-f: Set score filter" << std::endl
              << "-s: Set max number of prediction per word" << std::endl
              << "-h: Show this help" << std::endl;
}

float score(const libime::LanguageModel &model, std::string_view w,
            std::string_view w2) {
    auto state = model.nullState();
    return model.wordsScore(state, std::vector<std::string_view>{w, w2});
}

int main(int argc, char *argv[]) {
    int c;
    unsigned long maxSize = 15;
    float filter =
        std::log10(libime::DEFAULT_LANGUAGE_MODEL_UNKNOWN_PROBABILITY_PENALTY) +
        1;
    while ((c = getopt(argc, argv, "f:s:h")) != -1) {
        switch (c) {
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

    using namespace libime;
    LanguageModel model(argv[optind]);
    DATrie<float> trie;

    std::ifstream fin;
    std::istream *in;
    if (strcmp(argv[optind + 1], "-") == 0) {
        in = &std::cin;
    } else {
        fin.open(argv[optind + 1], std::ios::in | std::ios::binary);
        in = &fin;
    }

    std::string line;
    std::unordered_map<std::string, std::unordered_map<std::string, float>>
        word;
    auto isSpaceCheck = boost::is_any_of(" \n\t\r\v\f");
    int grams = -1;
    while (!in->eof()) {
        if (!std::getline(*in, line)) {
            break;
        }

        boost::trim_if(line, isSpaceCheck);

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
        std::vector<std::string> tokens;
        boost::split(tokens, line, isSpaceCheck);

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
        std::sort(result.begin(), result.end(),
                  [](const auto &lhs, const auto &rhs) {
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
    if (strcmp(argv[optind + 2], "-") == 0) {
        out = &std::cout;
    } else {
        fout.open(argv[optind + 2], std::ios::out | std::ios::binary);
        out = &fout;
    }
    trie.save(*out);
    return 0;
}
