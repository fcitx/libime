/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/historybigram.h"
#include <exception>
#include <fstream>
#include <unistd.h>
#include <iostream>

void usage(const char *argv0) {
    std::cout << "Usage: " << argv0 << " [-c] <source> <dest>" << std::endl
              << "-c: Compile text to binary" << std::endl
              << "-h: Show this help" << std::endl;
}

int main(int argc, char *argv[]) {
    bool compile = false;
    int c;
    while ((c = getopt(argc, argv, "ch")) != -1) {
        switch (c) {
        case 'c':
            compile = true;
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
    HistoryBigram history;

    try {
        std::ifstream in(argv[optind], std::ios::in | std::ios::binary);
        if (compile) {
            history.loadText(in);
        } else {
            history.load(in);
        }

        std::ofstream fout;
        std::ostream *out;
        if (strcmp(argv[optind + 1], "-") == 0) {
            out = &std::cout;
        } else {
            fout.open(argv[optind + 1], std::ios::out | std::ios::binary);
            out = &fout;
        }
        if (compile) {
            history.save(*out);
        } else {
            history.dump(*out);
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
