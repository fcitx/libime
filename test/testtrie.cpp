#include "libime/datrie.h"
#include <cassert>
#include <fcntl.h>
#include <fstream>
#include <iostream>
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
        tree.update(key, [count, &map, &key](TestTrie::value_type v) -> TestTrie::value_type {
            if (v != 0) {
                // this is a key inserted twice
                assert(map.find(key) != map.end());
            }
            // std::cout << key << " " << v << " " << count << std::endl;
            return count;
        });
        assert(tree.exactMatchSearch(key) == count);
        count++;
    }

    std::vector<TestTrie::value_type> d;
    d.resize(tree.size());
    tree.dump(d.data(), d.size());

    assert(tree.size() == map.size());
    for (auto &p : map) {
        // std::cout << p.first << " " << tree.exactMatchSearch(p.first) << " " << p.second <<
        // std::endl;
        assert(tree.exactMatchSearch(p.first) == p.second);
    }

    std::string tempKey;
    size_t foreach_count = 0;
    tree.foreach ([&tree, &map, &tempKey, &foreach_count](TestTrie::value_type value, size_t len,
                                                          uint64_t pos) {
        (void)value;
        tree.suffix(tempKey, len, pos);
        assert(map.find(tempKey) != map.end());
        assert(tree.exactMatchSearch(tempKey) == value);
        assert(map[tempKey] == value);
        tree.update(tempKey, [](int32_t v) { return v + 1; });
        foreach_count++;
        return true;
    });

    tree.erase(map.begin()->first);
    assert(tree.size() == foreach_count - 1);

    tree.save("trie_data");

    tree.clear();

    assert(!tree.erase(map.begin()->first));
    assert(tree.size() == 0);
    decltype(tree) trie2("trie_data");
    swap(tree, trie2);

    foreach_count = 0;
    tree.foreach ([&tree, &map, &tempKey, &foreach_count](int32_t value, size_t len, uint64_t pos) {
        (void)value;
        tree.suffix(tempKey, len, pos);
        assert(map.find(tempKey) != map.end());
        assert(tree.exactMatchSearch(tempKey) == value);
        assert(map[tempKey] + 1 == value);
        foreach_count++;
        return true;
    });

    assert(tree.size() == foreach_count);

    return 0;
}
