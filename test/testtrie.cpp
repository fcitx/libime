/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "libime/core/datrie.h"
#include <cstdint>
#include <cstring>
#include <fcitx-utils/log.h>

using namespace libime;

int main() {
    {
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
    }

    {
        DATrie<float> trie;
        trie.set("aaaa", 1);
        trie.set("aaab", 1);
        trie.set("aaac", 1);
        trie.set("aaad", 1);
        trie.set("aab", 1);
        FCITX_ASSERT(trie.size() == 5);
        trie.erase("aaaa");
        FCITX_ASSERT(trie.size() == 4);
        DATrie<float>::position_type pos = 0;
        auto result = trie.traverse("aaa", pos);
        auto nan1 = DATrie<float>::noValue();
        auto nan2 = DATrie<float>::noPath();
        // NaN != NaN, we must use memcmp to do this.
        // NOLINTBEGIN(bugprone-suspicious-memory-comparison)
        FCITX_ASSERT(memcmp(&nan1, &result, sizeof(float)) == 0);
        FCITX_ASSERT(trie.isNoValue(result));
        result = trie.traverse("aaae", pos);
        FCITX_ASSERT(memcmp(&nan2, &result, sizeof(float)) == 0);
        // NOLINTEND(bugprone-suspicious-memory-comparison)
        FCITX_ASSERT(trie.isNoPath(result));
        trie.erase(pos);
        FCITX_ASSERT(trie.size() == 4);
    }
    return 0;
}
