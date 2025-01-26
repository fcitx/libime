/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_DICTIONARY_H_
#define _FCITX_LIBIME_CORE_DICTIONARY_H_

#include <functional>
#include <libime/core/lattice.h>
#include <libime/core/libimecore_export.h>
#include <libime/core/segmentgraph.h>
#include <memory>
#include <unordered_set>

namespace libime {

class WordNode;

// The callback accepts the passed path that matches the word.
using GraphMatchCallback =
    std::function<void(const SegmentGraphPath &, WordNode &, float,
                       std::unique_ptr<LatticeNodeData>)>;

class LIBIMECORE_EXPORT Dictionary {
public:
    void
    matchPrefix(const SegmentGraph &graph, const GraphMatchCallback &callback,
                const std::unordered_set<const SegmentGraphNode *> &ignore = {},
                void *helper = nullptr) const;

protected:
    virtual void
    matchPrefixImpl(const SegmentGraph &graph,
                    const GraphMatchCallback &callback,
                    const std::unordered_set<const SegmentGraphNode *> &ignore,
                    void *helper) const = 0;
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_DICTIONARY_H_
