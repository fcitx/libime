/*
* Copyright (C) 2017~2017 by CSSlayer
* wengxt@gmail.com
*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2 of the
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
#include "testdir.h"
#include <fcitx-utils/log.h>
#include <set>
#include <sstream>
#include <string>
#include <unistd.h>

using namespace libime;

void testMatch(const TableBasedDictionary &dict, boost::string_view code,
               std::set<std::string> expect, bool exact) {
    std::set<std::string> actual;
    dict.matchWords(
        code, exact ? TableMatchMode::Exact : TableMatchMode::Prefix,
        [&actual](boost::string_view, boost::string_view word, uint32_t, PhraseFlag) {
            actual.insert(word.to_string());
            return true;
        });
    FCITX_ASSERT(expect == actual);
}

void testWubi() {

    std::string test = "KeyCode=abcdefghijklmnopqrstuvwxy\n"
                       "Length=4\n"
                       "Pinyin=@\n"
                       "[Rule]\n"
                       "e2=p11+p12+p21+p22\n"
                       "e3=p11+p21+p31+p32\n"
                       "a4=p11+p21+p31+n11\n"
                       "[Data]\n"
                       "xycq 统\n"
                       "yfh 计\n"
                       "nnkd 局\n";

    std::stringstream ss(test);

    try {
        libime::TableBasedDictionary table;
        table.load(ss, libime::TableFormat::Text);
        FCITX_ASSERT(table.hasRule());
        FCITX_ASSERT(table.hasPinyin());
        std::string key;
        FCITX_ASSERT(!table.generate("你好", key));
        FCITX_ASSERT(table.generate("统计局", key));
        FCITX_ASSERT(key == "xynn");

        FCITX_ASSERT(table.insert("wq", "你"));
        FCITX_ASSERT(table.insert("wqiy", "你"));
        FCITX_ASSERT(table.insert("v", "好"));
        FCITX_ASSERT(table.insert("vbg", "好"));

        table.save(LIBIME_BINARY_DIR "/test/testtable.dict");
        table.statistic();

        table.load(LIBIME_BINARY_DIR "/test/testtable.dict");
        table.statistic();
        // table.dump(std::cout);

        std::string key2;
        FCITX_ASSERT(table.generate("统计局", key2));
        FCITX_ASSERT(key == key2);
        FCITX_ASSERT(table.generate("你好", key2));
        std::cout << key2 << std::endl;
        FCITX_ASSERT(key2 == "wqvb");
        FCITX_ASSERT(table.insert("你好"));
        testMatch(table, "wqvb", {"你好"}, false);
        testMatch(table, "wqvb", {"你好"}, true);
        testMatch(table, "w", {"你", "你好"}, false);
        testMatch(table, "wq", {"你", "你好"}, false);
        testMatch(table, "w", {}, true);
        table.insert("wo", "我", PhraseFlag::Pinyin);
        testMatch(table, "w", {"你", "你好", "我"}, false);

        FCITX_ASSERT(table.reverseLookup("你") == "wqiy");
        FCITX_ASSERT(table.reverseLookup("好") == "vbg");
        table.statistic();
        table.save(std::cout, TableFormat::Text);
    } catch (std::ios_base::failure &e) {
        std::cout << e.what() << std::endl;
    }
}

void testCangjie() {

    std::string test = "KeyCode=abcdefghijklmnopqrstuvwxyz\n"
                       "Length=6\n"
                       "Prompt=&\n"
                       "[Data]\n"
                       "&a 日\n"
                       "&b 月\n"
                       "&c 金\n"
                       "a 日\n"
                       "a 曰\n"
                       "aa 昌\n"
                       "aaa 晶\n"
                       "abac 暝\n";

    std::stringstream ss(test);

    try {
        libime::TableBasedDictionary table;
        table.load(ss, libime::TableFormat::Text);
        table.save(std::cout, TableFormat::Text);
        FCITX_ASSERT(!table.hasRule());
        std::string key;
        FCITX_ASSERT(!table.generate("你好", key));
        FCITX_ASSERT(table.hint("abac") == "日月日金");
    } catch (std::ios_base::failure &e) {
        std::cout << e.what() << std::endl;
    }
}

int main() {
    testWubi();
    testCangjie();

    return 0;
}
