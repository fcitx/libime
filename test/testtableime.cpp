/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/languagemodel.h"
#include "libime/core/userlanguagemodel.h"
#include "libime/table/tablebaseddictionary.h"
#include "libime/table/tablecontext.h"
#include "libime/table/tabledecoder.h"
#include "libime/table/tableoptions.h"
#include "testdir.h"
#include "testutils.h"
#include <cstddef>
#include <fcitx-utils/log.h>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>

using namespace libime;

class TestLmResolver : public LanguageModelResolver {
public:
    TestLmResolver(std::string_view path) : path_(path) {}

protected:
    std::string
    languageModelFileNameForLanguage(const std::string &language) override {
        if (language == "zh_CN") {
            return path_;
        }
        return {};
    }

private:
    std::string path_;
};

int main() {
    fcitx::Log::setLogRule("*=5");
    TestLmResolver lmresolver(LIBIME_BINARY_DIR "/data/sc.lm");
    auto lm = lmresolver.languageModelFileForLanguage("zh_CN");
    TableBasedDictionary dict;
    UserLanguageModel model(lm);
    dict.load(LIBIME_BINARY_DIR "/data/wbpy.main.dict");
    TableOptions options;
    options.setLanguageCode("zh_CN");
    options.setAutoSelect(true);
    options.setAutoSelectLength(-1);
    options.setNoMatchAutoSelectLength(-1);
    options.setNoSortInputLength(2);
    options.setAutoRuleSet({"e2"});
    options.setMatchingKey('z');
    options.setOrderPolicy(OrderPolicy::Freq);
    dict.setTableOptions(options);
    TableContext c(dict, model);
    auto printTime = [](int t) {
        std::cout << "Time: " << t / 1000000.0 << " ms" << std::endl;
    };

    std::string word;
    while (std::cin >> word) {
        bool printAll = false;
        ScopedNanoTimer t(printTime);
        if (word == "back") {
            c.backspace();
        } else if (word == "reset") {
            c.clear();
        } else if (word.size() == 1 && ('0' <= word[0] && word[0] <= '9')) {
            size_t idx;
            if (word[0] == '0') {
                idx = 9;
            } else {
                idx = word[0] - '1';
            }
            if (c.candidates().size() >= idx) {
                c.select(idx);
            }
        } else if (word.size() == 1 && c.isValidInput(word[0])) {
            c.type(word);
        } else if (word == "all") {
            printAll = true;
        } else if (word == "commit") {
            c.autoSelect();
            c.learn();
            c.clear();
        }

        size_t count = 1;
        std::cout << "Preedit: " << c.preedit() << std::endl;
        for (const auto &candidate : c.candidates()) {
            std::cout << (count % 10) << ": ";
            for (const auto *node : candidate.sentence()) {
                std::cout
                    << node->word() << " "
                    << static_cast<const TableLatticeNode *>(node)->code();
            }
            std::cout << " " << candidate.score() << std::endl;
            count++;
            if (!printAll && count > 10) {
                break;
            }
        }
    }

    return 0;
}
