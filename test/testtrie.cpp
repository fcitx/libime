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
