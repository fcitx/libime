/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "libime/core/datrie.h"
#include <fcitx-utils/log.h>

using namespace libime;

int main() {
    DATrie<int32_t> trie;
    trie.set("aaaa", 1);
    trie.set("aaab", 1);
    trie.set("aaac", 1);
    trie.set("aaad", 1);
    trie.set("aab", 1);
    FCITX_ASSERT(trie.size() == 5);
    trie.erase("aaaa");
    FCITX_ASSERT(trie.size() == 4);
    DATrie<int32_t>::position_type pos = 0;
    auto result = trie.traverse("aaa", pos);
    FCITX_ASSERT(trie.isNoValue(result));
    trie.erase(pos);
    FCITX_ASSERT(trie.size() == 4);
    return 0;
}
