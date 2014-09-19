#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include "libime/radixtrie.h"
#include "libime/loudstrie.h"

#define myassert(EXPR) EXPR ? (void) 0 : abort()

size_t count = 0;
void counter(const smallvector& key, const std::string& s)
{
    count++;
}

using namespace libime;

int main(int argc, char* argv[]) {
    if (argc > 1) {
        int fd = open(argv[1], O_RDONLY);
        if (fd == -1) {
            return 1;
        }
        dup2(fd, 0);
    }

    RadixTrie<std::string> tree;
    std::string key, value;

    LoudsTrieBuilder builder;
    while (std::cin >> key >> value) {
         tree.add(key, value);
         builder.add(key.c_str(), key.c_str() + key.length());
    }

    tree.prefix_foreach(std::string("a"), counter);
    count = 0;
    tree.foreach(counter);
    assert(count == tree.size());

    std::cout << tree.size() << " " << count << std::endl;
    //sleep(5);

    std::ofstream fout;
    fout.open("test");
    builder.build(fout);

    return 0;
}
