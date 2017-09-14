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

#include "libime/table/tablebaseddictionary.h"
#include "libime/table/tablecontext.h"
#include "libime/table/tableime.h"
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

class TestTableIME : public TableIME {
public:
    TestTableIME(LanguageModelResolver *r, boost::string_view sys,
                 boost::string_view usr)
        : TableIME(r), sys_(sys), usr_(usr) {}

protected:
    TableBasedDictionary *requestDictImpl(boost::string_view name) override {
        if (name != "wbx") {
            return nullptr;
        }
        auto dict = new TableBasedDictionary;
        dict->load(sys_.c_str(), TableFormat::Binary);
        TableOptions options;
        options.setLanguageCode("zh_CN");
        options.setAutoSelect(true);
        options.setAutoSelectLength(-1);
        options.setNoMatchAutoSelectLength(0);
        dict->setTableOptions(options);
        return dict;
    }

    void saveDictImpl(TableBasedDictionary *dict) override {
        if (usr_.empty()) {
            return;
        }
        dict->saveUser(usr_.c_str(), TableFormat::Binary);
    }

private:
    std::string sys_, usr_;
};

int main() {
    TestLmResolver lmresolver(LIBIME_BINARY_DIR "/data/sc.lm");
    TestTableIME ime(&lmresolver, LIBIME_BINARY_DIR "/data/wbx.main.dict",
                     LIBIME_BINARY_DIR "/data/wbx.user.dict");
    auto dict = ime.requestDict("wbx");
    TableContext c(*dict, *ime.languageModelForDictionary(dict));
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
        }

        size_t count = 1;
        std::cout << "Preedit: " << c.preedit() << std::endl;
        for (auto &candidate : c.candidates()) {
            std::cout << (count % 10) << ": ";
            for (auto node : candidate.sentence()) {
                std::cout << node->word() << " " << node->auxiliary();
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
