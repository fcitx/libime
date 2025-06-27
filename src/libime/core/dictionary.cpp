/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "dictionary.h"
#include <unordered_set>
#include "segmentgraph.h"

void libime::Dictionary::matchPrefix(
    const SegmentGraph &graph, const GraphMatchCallback &callback,
    const std::unordered_set<const SegmentGraphNode *> &ignore,
    void *helper) const {
    matchPrefixImpl(graph, callback, ignore, helper);
}
