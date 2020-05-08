/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_DICTIONARY_H_
#define _FCITX_LIBIME_CORE_DICTIONARY_H_

#include "lattice.h"
#include "libimecore_export.h"
#include "segmentgraph.h"
#include <functional>
#include <string_view>

namespace libime {

class WordNode;

// The callback accepts the passed path that matches the word.
typedef std::function<void(const SegmentGraphPath &, WordNode &, float,
                           std::unique_ptr<LatticeNodeData>)>
    GraphMatchCallback;

class LIBIMECORE_EXPORT Dictionary {
public:
    void
    matchPrefix(const SegmentGraph &graph, const GraphMatchCallback &callback,
                const std::unordered_set<const SegmentGraphNode *> &ignore = {},
                void *helper = nullptr) const {
        matchPrefixImpl(graph, callback, ignore, helper);
    }

protected:
    virtual void
    matchPrefixImpl(const SegmentGraph &graph,
                    const GraphMatchCallback &callback,
                    const std::unordered_set<const SegmentGraphNode *> &ignore,
                    void *helper) const = 0;
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_DICTIONARY_H_
