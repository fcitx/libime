//
// Copyright (C) 2018~2018 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include "triedictionary.h"

#include "libime/core/datrie.h"
#include "libime/core/lattice.h"
#include "libime/core/lrucache.h"
#include "libime/core/utils.h"
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/unordered_map.hpp>
#include <boost/utility/string_view.hpp>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <queue>
#include <type_traits>

namespace libime {

class TrieDictionaryPrivate : fcitx::QPtrHolder<TrieDictionary> {
public:
    TrieDictionaryPrivate(TrieDictionary *q)
        : fcitx::QPtrHolder<TrieDictionary>(q) {}

    FCITX_DEFINE_SIGNAL_PRIVATE(TrieDictionary, dictionaryChanged);

    boost::ptr_vector<typename TrieDictionary::TrieType> tries_;
};

TrieDictionary::TrieDictionary()
    : d_ptr(std::make_unique<TrieDictionaryPrivate>(this)) {
    addEmptyDict();
    addEmptyDict();
}

TrieDictionary::~TrieDictionary() {}

void TrieDictionary::addEmptyDict() {
    FCITX_D();
    d->tries_.push_back(new TrieType);
}

void TrieDictionary::remove(size_t idx) {
    FCITX_D();
    if (idx <= UserDict) {
        throw std::invalid_argument("User Dict not allow to be removed");
    }
    d->tries_.erase(d->tries_.begin() + idx);
}

void TrieDictionary::removeAll() {
    FCITX_D();
    d->tries_.erase(d->tries_.begin() + UserDict + 1, d->tries_.end());
}

void TrieDictionary::clear(size_t idx) {
    FCITX_D();
    d->tries_[idx].clear();
}

const TrieDictionary::TrieType *TrieDictionary::trie(size_t idx) const {
    FCITX_D();
    return &d->tries_[idx];
}

TrieDictionary::TrieType *TrieDictionary::mutableTrie(size_t idx) {
    FCITX_D();
    return &d->tries_[idx];
}

size_t TrieDictionary::dictSize() const {
    FCITX_D();
    return d->tries_.size();
}

void TrieDictionary::addWord(size_t idx, boost::string_view key, float cost) {
    FCITX_D();
    d->tries_[idx].set(key.data(), key.size(), cost);
    emit<TrieDictionary::dictionaryChanged>(idx);
}
} // namespace libime
