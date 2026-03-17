/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <unistd.h>
#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <fcitx-utils/log.h>
#include "libime/core/utils.h"
#include "libime/core/utils_p.h"
#include "libime/pinyin/pinyindictionary.h"

namespace {

void usage(const char *argv0) {
    std::cout << "Usage: " << argv0 << " [-d] <source> <dest>\n"
              << "-d: Dump binary to text\n"
              << "-v: Show debug message\n"
              << "-h: Show this help\n";
}

} // namespace

int main(int argc, char *argv[]) {

    bool dump = false;
    int c;
    while ((c = getopt(argc, argv, "dhv")) != -1) {
        switch (c) {
        case 'd':
            dump = true;
            break;
        case 'v':
            fcitx::Log::setLogRule("libime=5");
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (optind + 2 != argc) {
        usage(argv[0]);
        return 1;
    }
    using namespace libime;
    PinyinDictionary dict;

    try {
        auto t0 = std::chrono::high_resolution_clock::now();

        std::ifstream in(argv[optind], std::ios::in | std::ios::binary);
        if (!in) {
            std::cerr << "Error: Failed to open input file: " << argv[optind]
                      << '\n';
            return 1;
        }
        dict.load(PinyinDictionary::SystemDict, in,
                  dump ? PinyinDictFormat::Binary : PinyinDictFormat::Text);
        LIBIME_DEBUG() << "Load pinyin dict: " << millisecondsTill(t0);
    } catch (const std::exception &e) {
        std::cerr << "Exception happened when loading input file "
                  << argv[optind] << ": " << e.what() << '\n';
        return 1;
    }

    std::ofstream fout;
    std::ostream *out;
    std::string_view outputFile(argv[optind + 1]);
    if (outputFile == "-") {
        out = &std::cout;
    } else {
        fout.open(std::string(outputFile), std::ios::out | std::ios::binary);
        if (!fout) {
            std::cerr << "Error: Failed to open output file: " << outputFile
                      << '\n';
            return 1;
        }
        out = &fout;
    }

    try {
        dict.save(PinyinDictionary::SystemDict, *out,
                  dump ? PinyinDictFormat::Text : PinyinDictFormat::Binary);
    } catch (const std::exception &e) {
        std::cerr << "Exception happened when saving output file " << outputFile
                  << ": " << e.what() << '\n';
        return 1;
    }
    return 0;
}
