/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

Lattice::Lattice() {}

Lattice::Lattice(Lattice &&other) : d_ptr(std::move(other.d_ptr)) {}

Lattice::Lattice(LatticePrivate *d) : d_ptr(d) {}

Lattice::~Lattice() {}

Lattice &Lattice::operator=(Lattice &&other) {
    d_ptr = std::move(other.d_ptr);
    return *this;
}

size_t Lattice::sentenceSize() const {
    FCITX_D();
    return d->nbests.size();
}

const SentenceResult &Lattice::sentence(size_t idx) const {
    FCITX_D();
    return d->nbests[idx];
}

Lattice::NodeRange Lattice::nodes(const SegmentGraphNode *node) const {
    FCITX_D();
    auto iter = d->lattice.find(node);
    if (iter == d->lattice.end()) {
        return {};
    }
    return {iter->second.begin(), iter->second.end()};
}
}
