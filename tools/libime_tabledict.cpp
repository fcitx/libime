/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/table/tablebaseddictionary.h"
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <tuple>

void usage(const char *argv0) {
    std::cout << "Usage: " << argv0 << " [-du] <source> <dest>" << std::endl
              << "-d: Dump binary to text" << std::endl
              << "-u: User dict" << std::endl
              << "-h: Show this help" << std::endl;
}

int main(int argc, char *argv[]) {

    bool dump = false;
    bool user = false;
    int c;
    while ((c = getopt(argc, argv, "dhu")) != -1) {
        switch (c) {
        case 'd':
            dump = true;
            break;
        case 'u':
            user = true;
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
    TableBasedDictionary dict;

    std::ifstream fin;
    std::istream *in;
    if (strcmp(argv[optind], "-") == 0) {
        in = &std::cin;
    } else {
        fin.open(argv[optind], std::ios::in | std::ios::binary);
        in = &fin;
    }

    if (user) {
        dict.loadUser(*in, dump ? TableFormat::Binary : TableFormat::Text);
    } else {
        dict.load(*in, dump ? TableFormat::Binary : TableFormat::Text);
    }

    std::ofstream fout;
    std::ostream *out;
    if (strcmp(argv[optind + 1], "-") == 0) {
        out = &std::cout;
    } else {
        fout.open(argv[optind + 1], std::ios::out | std::ios::binary);
        out = &fout;
    }

    if (user) {
        dict.saveUser(*out, dump ? TableFormat::Text : TableFormat::Binary);
    } else {
        dict.save(*out, dump ? TableFormat::Text : TableFormat::Binary);
    }
    return 0;
}
