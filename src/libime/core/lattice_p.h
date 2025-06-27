/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_LATTICE_P_H_
#define _FCITX_LIBIME_CORE_LATTICE_P_H_

#include <unordered_map>
#include <vector>
#include <boost/ptr_container/ptr_vector.hpp>
#include <libime/core/lattice.h>
#include <libime/core/segmentgraph.h>

namespace libime {

using LatticeMap = std::unordered_map<const SegmentGraphNode *,
                                      boost::ptr_vector<LatticeNode>>;

class LatticePrivate {
public:
    LatticeMap lattice_;

    std::vector<SentenceResult> nbests_;
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_LATTICE_P_H_
