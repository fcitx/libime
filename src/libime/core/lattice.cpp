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

#include "lattice.h"
#include "lattice_p.h"

namespace libime {

WordNode::WordNode(WordNode &&other) noexcept(
    std::is_nothrow_move_constructible<std::string>::value) = default;
WordNode &WordNode::operator=(WordNode &&other) noexcept(
    std::is_nothrow_move_assignable<std::string>::value) = default;

Lattice::Lattice() : d_ptr(std::make_unique<LatticePrivate>()) {}

FCITX_DEFINE_DEFAULT_DTOR_AND_MOVE(Lattice)

size_t Lattice::sentenceSize() const {
    FCITX_D();
    return d->nbests_.size();
}

const SentenceResult &Lattice::sentence(size_t idx) const {
    FCITX_D();
    return d->nbests_[idx];
}

Lattice::NodeRange Lattice::nodes(const SegmentGraphNode *node) const {
    FCITX_D();
    auto iter = d->lattice_.find(node);
    if (iter == d->lattice_.end()) {
        return {};
    }
    return {iter->second.begin(), iter->second.end()};
}

void Lattice::clear() {
    FCITX_D();
    d->lattice_.clear();
    d->nbests_.clear();
}

void Lattice::discardNode(
    const std::unordered_set<const SegmentGraphNode *> &nodes) {
    FCITX_D();
    for (auto node : nodes) {
        d->lattice_.erase(node);
    }
    for (auto &p : d->lattice_) {
        p.second.erase_if([&nodes](const LatticeNode &node) {
            return nodes.count(node.from());
        });
    }
}
}
