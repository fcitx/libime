/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "autophrasedict.h"
#include "libime/core/utils.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <cstddef>
#include <cstdint>
#include <fcitx-utils/macros.h>
#include <functional>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

namespace libime {

struct AutoPhrase {
    AutoPhrase(std::string entry, uint32_t hit)
        : entry_(std::move(entry)), hit_(hit) {}

    std::string_view entry() const { return entry_; }

    std::string entry_;
    uint32_t hit_ = 0;
};

class AutoPhraseDictPrivate {
    using item_list = boost::multi_index_container<
        AutoPhrase,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<
                boost::multi_index::const_mem_fun<AutoPhrase, std::string_view,
                                                  &AutoPhrase::entry>>>>;

public:
    using iterator = item_list::iterator;

    AutoPhraseDictPrivate(size_t maxItem) : maxItems_(maxItem) {}
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(AutoPhraseDictPrivate);

    item_list il_;
    std::size_t maxItems_;
};

AutoPhraseDict::AutoPhraseDict(size_t maxItems)
    : d_ptr(std::make_unique<AutoPhraseDictPrivate>(maxItems)) {}

AutoPhraseDict::AutoPhraseDict(size_t maxItems, std::istream &in)
    : AutoPhraseDict(maxItems) {
    load(in);
}

FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(AutoPhraseDict)

void AutoPhraseDict::insert(const std::string &entry, uint32_t value) {
    FCITX_D();
    auto &il = d->il_;
    auto p = il.push_front(AutoPhrase{entry, value});

    auto iter = p.first;
    if (!p.second) {
        il.relocate(il.begin(), p.first);
        iter = il.begin();
    }
    if (value == 0) {
        il.modify(iter, [](AutoPhrase &phrase) { phrase.hit_ += 1; });
    }
    if (il.size() > d->maxItems_) {
        il.pop_back();
    }
}

bool AutoPhraseDict::search(
    std::string_view s,
    const std::function<bool(std::string_view, uint32_t)> &callback) const {
    FCITX_D();
    const auto &idx = d->il_.get<1>();
    auto iter = idx.lower_bound(s);
    while (iter != idx.end() && boost::starts_with(iter->entry(), s)) {
        if (!callback(iter->entry(), iter->hit_)) {
            return false;
        }
        ++iter;
    }
    return true;
}

uint32_t AutoPhraseDict::exactSearch(std::string_view s) const {
    FCITX_D();
    const auto &idx = d->il_.get<1>();
    auto iter = idx.find(s);
    if (iter == idx.end()) {
        return 0;
    }
    return iter->hit_;
}

void AutoPhraseDict::erase(std::string_view s) {
    FCITX_D();
    auto &idx = d->il_.get<1>();
    idx.erase(s);
}

void AutoPhraseDict::clear() {
    FCITX_D();
    d->il_.clear();
}

bool AutoPhraseDict::empty() const {
    FCITX_D();
    return d->il_.empty();
}

void AutoPhraseDict::load(std::istream &in) {
    uint32_t size = 0;
    throw_if_io_fail(unmarshall(in, size));
    while (size--) {
        std::string text;
        uint32_t hit = 0;
        throw_if_io_fail(unmarshallString(in, text));
        throw_if_io_fail(unmarshall(in, hit));
        insert(text, hit);
    }
}

void AutoPhraseDict::save(std::ostream &out) {
    FCITX_D();
    uint32_t size = d->il_.size();
    throw_if_io_fail(marshall(out, size));
    for (const auto &phrase : d->il_ | boost::adaptors::reversed) {
        throw_if_io_fail(marshallString(out, phrase.entry_));
        throw_if_io_fail(marshall(out, phrase.hit_));
    }
}
} // namespace libime
