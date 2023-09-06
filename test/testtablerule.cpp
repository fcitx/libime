/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/table/tablerule.h"
#include <fcitx-utils/log.h>

using namespace libime;

int main() {
    {
        TableRule rule("e2=p11+p1z+p21+p2y+p2z", 5);
        FCITX_ASSERT(rule.codeLength() == 5);
        FCITX_ASSERT(rule.entries().size() == 5);
        FCITX_ASSERT(rule.flag() == TableRuleFlag::LengthEqual);
        FCITX_ASSERT(rule.toString() == "e2=p11+p1z+p21+p2y+p2z");
        FCITX_ASSERT(rule.phraseLength() == 2);
        FCITX_ASSERT(rule.entries()[0].flag() == TableRuleEntryFlag::FromFront);
        FCITX_ASSERT(rule.entries()[0].index() == 1);
        FCITX_ASSERT(rule.entries()[0].character() == 1);
        FCITX_ASSERT(rule.entries()[1].flag() == TableRuleEntryFlag::FromFront);
        FCITX_ASSERT(rule.entries()[1].index() == -1);
        FCITX_ASSERT(rule.entries()[1].character() == 1);
        FCITX_ASSERT(rule.entries()[2].flag() == TableRuleEntryFlag::FromFront);
        FCITX_ASSERT(rule.entries()[2].index() == 1);
        FCITX_ASSERT(rule.entries()[2].character() == 2);
        FCITX_ASSERT(rule.entries()[3].flag() == TableRuleEntryFlag::FromFront);
        FCITX_ASSERT(rule.entries()[3].index() == -2);
        FCITX_ASSERT(rule.entries()[3].character() == 2);
        FCITX_ASSERT(rule.entries()[4].flag() == TableRuleEntryFlag::FromFront);
        FCITX_ASSERT(rule.entries()[4].index() == -1);
        FCITX_ASSERT(rule.entries()[4].character() == 2);
    }
    {
        TableRule rule("E3=P11+P1Z+P21+P2Z+P3Z", 5);
        FCITX_ASSERT(rule.codeLength() == 5);
        FCITX_ASSERT(rule.entries().size() == 5);
        FCITX_ASSERT(rule.flag() == TableRuleFlag::LengthEqual);
        FCITX_ASSERT(rule.toString() == "e3=p11+p1z+p21+p2z+p3z");
        FCITX_ASSERT(rule.phraseLength() == 3);
        FCITX_ASSERT(rule.entries()[0].flag() == TableRuleEntryFlag::FromFront);
        FCITX_ASSERT(rule.entries()[0].index() == 1);
        FCITX_ASSERT(rule.entries()[0].character() == 1);
        FCITX_ASSERT(rule.entries()[1].flag() == TableRuleEntryFlag::FromFront);
        FCITX_ASSERT(rule.entries()[1].index() == -1);
        FCITX_ASSERT(rule.entries()[1].character() == 1);
        FCITX_ASSERT(rule.entries()[2].flag() == TableRuleEntryFlag::FromFront);
        FCITX_ASSERT(rule.entries()[2].index() == 1);
        FCITX_ASSERT(rule.entries()[2].character() == 2);
        FCITX_ASSERT(rule.entries()[3].flag() == TableRuleEntryFlag::FromFront);
        FCITX_ASSERT(rule.entries()[3].index() == -1);
        FCITX_ASSERT(rule.entries()[3].character() == 2);
        FCITX_ASSERT(rule.entries()[4].flag() == TableRuleEntryFlag::FromFront);
        FCITX_ASSERT(rule.entries()[4].index() == -1);
        FCITX_ASSERT(rule.entries()[4].character() == 3);
    }
    return 0;
}