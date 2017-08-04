#include "libime/datrie.h"
#include <fcitx-utils/log.h>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <unistd.h>
#include <unordered_map>

using namespace libime;

int main(int argc, char *argv[]) {
    if (argc > 1) {
        int fd = open(argv[1], O_RDONLY);
        if (fd == -1) {
            return 1;
        }
        dup2(fd, 0);
    }

    typedef DATrie<int32_t> TestTrie;
    TestTrie tree;
    std::string key;
    std::unordered_map<std::string, int32_t> map;

    int count = 1;
    // key can be same as other
    while (std::cin >> key) {
        map[key] = count;
        tree.update(key, [count, &map, &key](
                             TestTrie::value_type v) -> TestTrie::value_type {
            if (v != 0) {
                // this is a key inserted twice
                FCITX_ASSERT(map.find(key) != map.end());
            }
            // std::cout << key << " " << v << " " << count << std::endl;
            return count;
        });
        FCITX_ASSERT(tree.exactMatchSearch(key) == count);
        count++;
    }

    std::vector<TestTrie::value_type> d;
    d.resize(tree.size());
    tree.dump(d.data(), d.size());

    FCITX_ASSERT(tree.size() == map.size());
    for (auto &p : map) {
        // std::cout << p.first << " " << tree.exactMatchSearch(p.first) << " "
        // << p.second <<
        // std::endl;
        FCITX_ASSERT(tree.exactMatchSearch(p.first) == p.second);
    }

    std::string tempKey;
    size_t foreach_count = 0;
    tree.foreach ([&tree, &map, &tempKey, &foreach_count](
        TestTrie::value_type value, size_t len, uint64_t pos) {
        (void)value;
        tree.suffix(tempKey, len, pos);
        FCITX_ASSERT(map.find(tempKey) != map.end());
        FCITX_ASSERT(tree.exactMatchSearch(tempKey) == value);
        FCITX_ASSERT(map[tempKey] == value);
        tree.update(tempKey, [](int32_t v) { return v + 1; });
        foreach_count++;
        return true;
    });

    tree.erase(map.begin()->first);
    FCITX_ASSERT(tree.size() == foreach_count - 1);

    tree.save("trie_data");

    tree.clear();

    FCITX_ASSERT(!tree.erase(map.begin()->first));
    FCITX_ASSERT(tree.size() == 0);
    decltype(tree) trie2("trie_data");
    swap(tree, trie2);

    foreach_count = 0;
    tree.foreach ([&tree, &map, &tempKey,
                   &foreach_count](int32_t value, size_t len, uint64_t pos) {
        (void)value;
        tree.suffix(tempKey, len, pos);
        FCITX_ASSERT(map.find(tempKey) != map.end());
        FCITX_ASSERT(tree.exactMatchSearch(tempKey) == value);
        FCITX_ASSERT(map[tempKey] + 1 == value);
        foreach_count++;
        return true;
    });

    FCITX_ASSERT(tree.size() == foreach_count);

    return 0;
}
