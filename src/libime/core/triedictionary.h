/*
 * SPDX-FileCopyrightText: 2018-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _LIBIME_LIBIME_CORE_TRIEDICTIONARY_H_
#define _LIBIME_LIBIME_CORE_TRIEDICTIONARY_H_

#include <cstddef>
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/macros.h>
#include <libime/core/datrie.h>
#include <libime/core/dictionary.h>
#include <libime/core/libimecore_export.h>
#include <memory>
#include <string_view>

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

    /**
     * Remove all dictionary from given index.
     *
     * @param idx the index need to be within [UserDict + 1, dictSize())
     * @since 1.0.10
     */
    void removeFrom(size_t idx);

    // Clear dictionary.
    void clear(size_t idx);

    const TrieType *trie(size_t idx) const;

    /**
     * Set trie from external source.
     *
     * There is no validation on the data within it, subclass may expect a
     * certain way of organize the key and value.
     *
     * @param idx the index need to be within [0, dictSize())
     * @param trie new trie.
     * @since 1.1.7
     */
    void setTrie(size_t idx, TrieType trie);

    // Total number to dictionary.
    size_t dictSize() const;

    FCITX_DECLARE_SIGNAL(TrieDictionary, dictionaryChanged, void(size_t));
    FCITX_DECLARE_SIGNAL(TrieDictionary, dictSizeChanged, void(size_t));

protected:
    TrieType *mutableTrie(size_t idx);
    void addWord(size_t idx, std::string_view key, float cost = 0.0F);
    bool removeWord(size_t idx, std::string_view key);

    std::unique_ptr<TrieDictionaryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TrieDictionary);
};
} // namespace libime

#endif // _LIBIME_LIBIME_CORE_TRIEDICTIONARY_H_
