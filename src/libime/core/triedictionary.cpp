/*
 * SPDX-FileCopyrightText: 2018-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "triedictionary.h"

#include "libime/core/datrie.h"
#include "libime/core/lattice.h"
#include "libime/core/lrucache.h"
#include "libime/core/utils.h"
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/unordered_map.hpp>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <queue>
#include <string_view>
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

void TrieDictionary::removeAll() {
    FCITX_D();
    for (auto i = UserDict + 1; i < d->tries_.size(); i++) {
        emit<TrieDictionary::dictionaryChanged>(i);
    }
    d->tries_.erase(d->tries_.begin() + UserDict + 1, d->tries_.end());
}

void TrieDictionary::clear(size_t idx) {
    FCITX_D();
    d->tries_[idx].clear();
    emit<TrieDictionary::dictionaryChanged>(idx);
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

void TrieDictionary::addWord(size_t idx, std::string_view key, float cost) {
    FCITX_D();
    d->tries_[idx].set(key.data(), key.size(), cost);
    emit<TrieDictionary::dictionaryChanged>(idx);
}
} // namespace libime
