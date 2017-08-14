#include "libime/tablebaseddictionary.h"
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
        [&actual](boost::string_view, boost::string_view word, float) {
            actual.insert(word.to_string());
            return true;
        });
    assert(expect == actual);
}

int main() {

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
        assert(table.hasRule());
        std::string key;
        assert(!table.generate("你好", key));
        assert(table.generate("统计局", key));
        assert(key == "xynn");

        assert(table.insert("wq", "你"));
        assert(table.insert("wqiy", "你"));
        assert(table.insert("v", "好"));
        assert(table.insert("vbg", "好"));

        table.save("data");
        table.statistic();

        table.load("data");
        table.statistic();
        // table.dump(std::cout);

        std::string key2;
        assert(table.generate("统计局", key2));
        assert(key == key2);
        assert(table.generate("你好", key2));
        std::cout << key2 << std::endl;
        assert(key2 == "wqvb");
        assert(table.insert("你好"));
        testMatch(table, "wqvb", {"你好"}, false);
        testMatch(table, "wqvb", {"你好"}, true);
        testMatch(table, "w", {"你", "你好"}, false);
        testMatch(table, "w", {}, true);
        table.statistic();
        table.save(std::cout, TableFormat::Text);
    } catch (std::ios_base::failure &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
