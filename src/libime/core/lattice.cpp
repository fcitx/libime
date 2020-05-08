/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
} // namespace libime
