/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <unistd.h>
#include <cstddef>
#include <exception>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include "libime/table/tablebaseddictionary.h"

namespace {

void usage(const char *argv0) {
    std::cout
        << "Usage: " << argv0 << " [-due] [-m <main dict>] <source> <dest>\n"
        << "-d: Dump binary to text\n"
        << "-u: User dict\n"
        << "-e: Extra dict\n"
        << "-m <path/to/main.dict>: Main dict to be used with extra dict\n"
        << "-h: Show this help\n";
}

} // namespace

int main(int argc, char *argv[]) {

    bool dump = false;
    bool user = false;
    bool extra = false;
    std::optional<std::string> extraMain = std::nullopt;
    int c;
    while ((c = getopt(argc, argv, "dhuem:")) != -1) {
        switch (c) {
        case 'd':
            dump = true;
            break;
        case 'u':
            user = true;
            break;
        case 'e':
            extra = true;
            break;
        case 'm':
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

    const auto inputFormat = dump ? TableFormat::Binary : TableFormat::Text;
    const auto outputFormat = dump ? TableFormat::Text : TableFormat::Binary;
    size_t extraIndex = 0;

    try {
        std::ifstream fin;
        std::istream *in;
        if (std::string_view{argv[optind]} == "-") {
            in = &std::cin;
        } else {
            fin.open(argv[optind], std::ios::in | std::ios::binary);
            if (!fin) {
                std::cerr << "Error: Failed to open input file: "
                          << argv[optind] << '\n';
                return 1;
            }
            in = &fin;
        }

        if (extra && extraMain) {
            std::ifstream extraIn(extraMain->c_str(),
                                  std::ios::in | std::ios::binary);
            if (!extraIn) {
                std::cerr << "Error: Failed to open extra main file: "
                          << *extraMain << '\n';
                return 1;
            }
            dict.load(extraIn, libime::TableFormat::Binary);
        }

        if (extra) {
            extraIndex = dict.loadExtra(*in, inputFormat);
        } else if (user) {
            dict.loadUser(*in, inputFormat);
        } else {
            dict.load(*in, inputFormat);
        }
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
        if (extra) {
            dict.saveExtra(extraIndex, *out, outputFormat);
        } else if (user) {
            dict.saveUser(*out, outputFormat);
        } else {
            dict.save(*out, outputFormat);
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception happened when saving output file " << outputFile
                  << ": " << e.what() << '\n';
        return 1;
    }
    return 0;
}
