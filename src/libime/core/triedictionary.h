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
#ifndef _LIBIME_LIBIME_CORE_TRIEDICTIONARY_H_
#define _LIBIME_LIBIME_CORE_TRIEDICTIONARY_H_

#include "libimecore_export.h"
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/macros.h>
#include <libime/core/datrie.h>
#include <libime/core/dictionary.h>
#include <memory>

namespace libime {

class TrieDictionaryPrivate;

class LIBIMECORE_EXPORT TrieDictionary : public Dictionary,
                                         public fcitx::ConnectableObject {
public:
    using TrieType = DATrie<float>;

    static const size_t SystemDict = 0;
    static const size_t UserDict = 1;
    explicit TrieDictionary();
    ~TrieDictionary();

    // Append a dictionary at the end.
    void addEmptyDict();

    // Remove all dictionary except system and user.
    void removeAll();

    // Clear dictionary.
    void clear(size_t idx);

    const DATrie<float> *trie(size_t idx) const;

    // Total number to dictionary.
    size_t dictSize() const;

    FCITX_DECLARE_SIGNAL(TrieDictionary, dictionaryChanged, void(size_t));

protected:
    DATrie<float> *mutableTrie(size_t idx);
    void addWord(size_t idx, boost::string_view result, float cost = 0.0f);

    std::unique_ptr<TrieDictionaryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TrieDictionary);
};
} // namespace libime

#endif // _LIBIME_LIBIME_CORE_TRIEDICTIONARY_H_
