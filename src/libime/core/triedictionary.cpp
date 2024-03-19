/*
 * SPDX-FileCopyrightText: 2018-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "triedictionary.h"

#include "libime/core/datrie.h"
#include <boost/ptr_container/ptr_vector.hpp>
#include <string_view>

namespace libime {

class TrieDictionaryPrivate : fcitx::QPtrHolder<TrieDictionary> {
public:
    TrieDictionaryPrivate(TrieDictionary *q)
        : fcitx::QPtrHolder<TrieDictionary>(q) {}

    FCITX_DEFINE_SIGNAL_PRIVATE(TrieDictionary, dictionaryChanged);
    FCITX_DEFINE_SIGNAL_PRIVATE(TrieDictionary, dictSizeChanged);

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
    emit<TrieDictionary::dictSizeChanged>(d->tries_.size());
}

void TrieDictionary::removeFrom(size_t idx) {
    FCITX_D();
    if (idx < UserDict + 1 || idx >= d->tries_.size()) {
        return;
    }

    for (auto i = idx; i < d->tries_.size(); i++) {
        emit<TrieDictionary::dictionaryChanged>(i);
    }
    d->tries_.erase(d->tries_.begin() + idx, d->tries_.end());
    emit<TrieDictionary::dictSizeChanged>(d->tries_.size());
}

void TrieDictionary::removeAll() { removeFrom(UserDict + 1); }

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

bool TrieDictionary::removeWord(size_t idx, std::string_view key) {
    FCITX_D();
    if (d->tries_[idx].erase(key.data(), key.size())) {
        emit<TrieDictionary::dictionaryChanged>(idx);
        return true;
    }
    return false;
}
} // namespace libime
