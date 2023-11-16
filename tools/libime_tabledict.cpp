/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/table/tablebaseddictionary.h"
#include <bits/getopt_core.h>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>

void usage(const char *argv0) {
    std::cout << "Usage: " << argv0 << " [-du] <source> <dest>" << std::endl
              << "-d: Dump binary to text" << std::endl
              << "-u: User dict" << std::endl
              << "-e <path/to/main.dict>: Extra dict" << std::endl
              << "-h: Show this help" << std::endl;
}

int main(int argc, char *argv[]) {

    bool dump = false;
    bool user = false;
    std::optional<std::string> extraMain = std::nullopt;
    int c;
    while ((c = getopt(argc, argv, "dhue:")) != -1) {
        switch (c) {
        case 'd':
            dump = true;
            break;
        case 'u':
            user = true;
            break;
        case 'e':
            extraMain = std::string(optarg);
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

    const auto inputFormat = dump ? TableFormat::Binary : TableFormat::Text;
    const auto outputFormat = dump ? TableFormat::Text : TableFormat::Binary;
    size_t extraIndex = 0;
    if (extraMain) {
        dict.load(extraMain->c_str(), libime::TableFormat::Binary);
        extraIndex = dict.loadExtra(*in, inputFormat);
    } else if (user) {
        dict.loadUser(*in, inputFormat);
    } else {
        dict.load(*in, inputFormat);
    }

    std::ofstream fout;
    std::ostream *out;
    if (strcmp(argv[optind + 1], "-") == 0) {
        out = &std::cout;
    } else {
        fout.open(argv[optind + 1], std::ios::out | std::ios::binary);
        out = &fout;
    }

    if (extraMain) {
        dict.saveExtra(extraIndex, *out, outputFormat);
    } else if (user) {
        dict.saveUser(*out, outputFormat);
    } else {
        dict.save(*out, outputFormat);
    }
    return 0;
}
