/*
 * SPDX-FileCopyrightText: 2018-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
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
    FCITX_DECLARE_SIGNAL(TrieDictionary, dictSizeChanged, void(size_t));

protected:
    DATrie<float> *mutableTrie(size_t idx);
    void addWord(size_t idx, std::string_view result, float cost = 0.0f);
    bool removeWord(size_t idx, std::string_view result);

    std::unique_ptr<TrieDictionaryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TrieDictionary);
};
} // namespace libime

#endif // _LIBIME_LIBIME_CORE_TRIEDICTIONARY_H_
