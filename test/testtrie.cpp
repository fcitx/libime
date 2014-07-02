#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include "libime/radix.h"

#define myassert(EXPR) EXPR ? (void) 0 : abort()

using namespace libime;

class A {
public:
    A(const std::string& _s): s(_s) {
    }
#if 0
    A(const A& a) : s(a.s) {
        std::cout << "AAA" << std::endl;
    }
    A& operator=(const A& a) {
        s = a.s;
        return *this;
    }
#else
    A(const A& a)= delete;
    A& operator=(const A& a) = delete;
#endif
    A(A&& a) : s(std::move(a.s)) {
        std::cout << "BBB" << std::endl;
    }
    A& operator=(A&& a) {
        s = std::move(a.s);
        return *this;
    }

    std::string s;
};
int main(int argc, char* argv[]) {
    int pagesize = sysconf(_SC_PAGESIZE);
    mallopt(M_TRIM_THRESHOLD, 5 * pagesize);
    std::vector<A> as;
//     FcitxRadixTree<uint8_t, void*> tree;
//     myassert(tree.add("abc", (void*)0x1));
//     myassert(tree.add("ab", (void*)0x1));
//     myassert(!tree.add("ab", (void*)0x1));
//     myassert(!tree.add("abc", (void*)0x1));
//     myassert(tree.remove("ab"));
//     myassert(!tree.add("abc", (void*)0x1));
//     myassert(tree.add("ab", (void*)0x1));
    if (argc > 1) {
        int fd = open(argv[1], O_RDONLY);
        if (fd == -1) {
            return 1;
        }
        dup2(fd, 0);
    }

    RadixTree<std::string> tree2;
    std::map<std::string, std::string> cmp;
    std::string key, value;
    while (std::cin >> key >> value) {
         tree2.add(key, value);
//          tree2.remove(key);
//          cmp[key] = value;
    }

    std::cout << tree2.size() << cmp.size()<< std::endl;
    sleep(5);
    return 0;
}
