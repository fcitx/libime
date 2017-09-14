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
#ifndef _FCITX_LIBIME_CORE_DICTIONARY_H_
#define _FCITX_LIBIME_CORE_DICTIONARY_H_

#include "libimecore_export.h"
#include "segmentgraph.h"
#include <boost/utility/string_view.hpp>
#include <functional>

namespace libime {

class WordNode;

// The callback accepts the passed path that matches the word.
typedef std::function<void(const SegmentGraphPath &, WordNode &, float,
                           boost::string_view)>
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
}

#endif // _FCITX_LIBIME_CORE_DICTIONARY_H_
