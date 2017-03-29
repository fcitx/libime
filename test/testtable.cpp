#include "libime/tablebaseddictionary.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

using namespace libime;

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
        libime::TableBasedDictionary table(
            ss, libime::TableBasedDictionary::TableFormat::Text);
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

        table = libime::TableBasedDictionary("data");
        table.statistic();
        // table.dump(std::cout);

        std::string key2;
        assert(table.generate("统计局", key2));
        assert(key == key2);
        assert(table.generate("你好", key2));
        std::cout << key2 << std::endl;
        assert(key2 == "wqvb");
        assert(table.insert("你好"));
        table.statistic();
        table.dump(std::cout);
    } catch (std::ios_base::failure &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
