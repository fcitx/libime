/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/historybigram.h"
#include <fstream>
#include <getopt.h>
#include <iostream>

void usage(const char *argv0) {
    std::cout << "Usage: " << argv0 << " <source> <dest>" << std::endl
              << "-h: Show this help" << std::endl;
}

int main(int argc, char *argv[]) {

    int c;
    while ((c = getopt(argc, argv, "h")) != -1) {
        switch (c) {
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
    HistoryBigram history;

    std::ifstream in(argv[optind], std::ios::in | std::ios::binary);
    history.load(in);

    std::ofstream fout;
    std::ostream *out;
    if (strcmp(argv[optind + 1], "-") == 0) {
        out = &std::cout;
    } else {
        fout.open(argv[optind + 1], std::ios::out | std::ios::binary);
        out = &fout;
    }
    history.dump(*out);
    return 0;
}
