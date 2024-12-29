/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "libime/table/autophrasedict.h"
#include <fcitx-utils/log.h>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>

using namespace libime;

void testSearch(const AutoPhraseDict &dict, std::string_view key,
                std::unordered_set<std::string> expect) {
    dict.search(key, [&expect](std::string_view entry, int) {
        expect.erase(std::string{entry});
        return true;
    });
    FCITX_ASSERT(expect.empty());
}

int main() {
    AutoPhraseDict dict(4);
    dict.insert("abc");
    dict.insert("ab");
    dict.insert("abcd");
    dict.insert("bcd");

    testSearch(dict, "a", {"abc", "ab", "abcd"});
    testSearch(dict, "ab", {"ab", "abc", "abcd"});
    testSearch(dict, "abc", {"abc", "abcd"});
    testSearch(dict, "abcd", {"abcd"});

    std::stringstream ss;
    dict.save(ss);

    AutoPhraseDict dict2(4);
    dict2.load(ss);
    testSearch(dict2, "a", {"abc", "ab", "abcd"});
    testSearch(dict2, "ab", {"ab", "abc", "abcd"});
    testSearch(dict2, "abc", {"abc", "abcd"});
    testSearch(dict2, "abcd", {"abcd"});
    testSearch(dict2, "", {"bcd", "ab", "abc", "abcd"});

    return 0;
}
