/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/utils.h"
#include "libime/pinyin/pinyindictionary.h"
#include <fcitx-utils/log.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

void usage(const char *argv0) {
    std::cout << "Usage: " << argv0 << " [-d] <source> <dest>" << std::endl
              << "-d: Dump binary to text" << std::endl
              << "-v: Show debug message" << std::endl
              << "-h: Show this help" << std::endl;
}

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

    auto t0 = std::chrono::high_resolution_clock::now();
    dict.load(PinyinDictionary::SystemDict, argv[optind],
              dump ? PinyinDictFormat::Binary : PinyinDictFormat::Text);
    LIBIME_DEBUG() << "Load pinyin dict: " << millisecondsTill(t0);

    std::ofstream fout;
    std::ostream *out;
    if (strcmp(argv[optind + 1], "-") == 0) {
        out = &std::cout;
    } else {
        fout.open(argv[optind + 1], std::ios::out | std::ios::binary);
        out = &fout;
    }
    dict.save(PinyinDictionary::SystemDict, *out,
              dump ? PinyinDictFormat::Text : PinyinDictFormat::Binary);
    return 0;
}
