/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <unistd.h>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include "libime/core/historybigram.h"

namespace {

void usage(const char *argv0) {
    std::cout << "Usage: " << argv0 << " [-c] <source> <dest>\n"
              << "-c: Compile text to binary\n"
              << "-h: Show this help\n";
}

} // namespace

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
        if (!in) {
            std::cerr << "Error: Failed to open input file: " << argv[optind]
                      << '\n';
            return 1;
        }
        if (compile) {
            history.loadText(in);
        } else {
            history.load(in);
        }

        std::ofstream fout;
        std::ostream *out;
        std::string_view outputFile(argv[optind + 1]);
        if (outputFile == "-") {
            out = &std::cout;
        } else {
            fout.open(std::string(outputFile),
                      std::ios::out | std::ios::binary);
            if (!fout) {
                std::cerr << "Error: Failed to open output file: " << outputFile
                          << '\n';
                return 1;
            }
            out = &fout;
        }
        if (compile) {
            history.save(*out);
        } else {
            history.dump(*out);
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
