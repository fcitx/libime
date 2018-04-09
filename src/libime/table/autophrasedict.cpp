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
#include "autophrasedict.h"
#include "libime/core/utils.h"
#include <boost/algorithm/string.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/range/adaptor/reversed.hpp>

namespace libime {

struct AutoPhrase {
    AutoPhrase(const std::string &entry, uint32_t hit)
        : entry_(entry), hit_(hit) {}

    boost::string_view entry() const { return entry_; }

    std::string entry_;
    uint32_t hit_ = 0;
};

class AutoPhraseDictPrivate {
    typedef boost::multi_index_container<
        AutoPhrase,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<
                boost::multi_index::const_mem_fun<
                    AutoPhrase, boost::string_view, &AutoPhrase::entry>>>>
        item_list;

public:
    using iterator = item_list::iterator;

    AutoPhraseDictPrivate(size_t maxItem) : maxItems_(maxItem) {}
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(AutoPhraseDictPrivate);

    item_list il;
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
    auto &il = d->il;
    auto p = il.push_front(AutoPhrase{entry, value});

    if (!p.second) {
        il.relocate(il.begin(), p.first);
        if (value == 0) {
            il.modify(il.begin(), [](AutoPhrase &phrase) { phrase.hit_ += 1; });
        }
    } else if (il.size() > d->maxItems_) {
        il.pop_back();
    }
}

bool AutoPhraseDict::search(
    boost::string_view s,
    std::function<bool(boost::string_view, uint32_t)> callback) const {
    FCITX_D();
    auto &idx = d->il.get<1>();
    auto iter = idx.lower_bound(s);
    while (iter != idx.end() && boost::starts_with(iter->entry(), s)) {
        if (!callback(iter->entry(), iter->hit_)) {
            return false;
        }
        ++iter;
    }
    return true;
}

uint32_t AutoPhraseDict::exactSearch(boost::string_view s) const {
    FCITX_D();
    auto &idx = d->il.get<1>();
    auto iter = idx.find(s);
    if (iter == idx.end()) {
        return 0;
    }
    return iter->hit_;
}

void AutoPhraseDict::erase(boost::string_view s) {
    FCITX_D();
    auto &idx = d->il.get<1>();
    idx.erase(s);
}

void AutoPhraseDict::clear() {
    FCITX_D();
    d->il.clear();
}

void AutoPhraseDict::load(std::istream &in) {
    uint32_t size;
    throw_if_io_fail(unmarshall(in, size));
    while (size--) {
        std::string text;
        uint32_t hit;
        throw_if_io_fail(unmarshallString(in, text));
        throw_if_io_fail(unmarshall(in, hit));
        insert(text, hit);
    }
}

void AutoPhraseDict::save(std::ostream &out) {
    FCITX_D();
    uint32_t size = d->il.size();
    throw_if_io_fail(marshall(out, size));
    for (auto &phrase : d->il | boost::adaptors::reversed) {
        throw_if_io_fail(marshallString(out, phrase.entry_));
        throw_if_io_fail(marshall(out, phrase.hit_));
    }
}
} // namespace libime
