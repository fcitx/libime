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
#include "libime/table/autophrasedict.h"
#include <fcitx-utils/log.h>
#include <sstream>
#include <unordered_set>

using namespace libime;

void testSearch(const AutoPhraseDict &dict, boost::string_view key,
                std::unordered_set<std::string> expect) {
    dict.search(key, [&expect](boost::string_view entry, int) {
        expect.erase(entry.to_string());
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
