/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "libime/core/userlanguagemodel.h"
#include "libime/table/tablebaseddictionary.h"
#include "libime/table/tablecontext.h"
#include "libime/table/tabledecoder.h"
#include "libime/table/tableoptions.h"
#include "testdir.h"
#include "testutils.h"
#include <fcitx-utils/log.h>

using namespace libime;

class TestLmResolver : public LanguageModelResolver {
public:
    TestLmResolver(boost::string_view path) : path_(path.to_string()) {}

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
        for (auto &candidate : c.candidates()) {
            std::cout << (count % 10) << ": ";
            for (auto node : candidate.sentence()) {
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
