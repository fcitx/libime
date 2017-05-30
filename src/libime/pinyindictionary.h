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
#ifndef _FCITX_LIBIME_UTF8_PINYINDICTIONARY_H_
#define _FCITX_LIBIME_UTF8_PINYINDICTIONARY_H_

#include "datrie.h"
#include "dictionary.h"
#include "libime_export.h"
#include "pinyinencoder.h"
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/macros.h>
#include <memory>

namespace libime {

enum class PinyinDictFormat { Text, Binary };

class PinyinDictionaryPrivate;

typedef std::function<bool(const char *encodedPinyin, const std::string &hanzi,
                           float cost)>
    PinyinMatchCallback;
class PinyinDictionary;

using PinyinTrie = DATrie<float>;

class LIBIME_EXPORT PinyinDictionary : public Dictionary,
                                       public fcitx::ConnectableObject {
public:
    static const size_t SystemDict = 0;
    static const size_t UserDict = 1;
    explicit PinyinDictionary();
    ~PinyinDictionary();

    void load(size_t idx, std::istream &in, PinyinDictFormat format);
    void load(size_t idx, const char *filename, PinyinDictFormat format);
    void addEmptyDict();

    void matchWords(const char *data, size_t size,
                    PinyinMatchCallback callback);

    void save(size_t idx, const char *filename);
    void save(size_t idx, std::ostream &out);
    void dump(size_t idx, std::ostream &out);
    void remove(size_t idx);
    const PinyinTrie *trie(size_t idx) const;
    size_t dictSize() const;

    void addWord(size_t idx, boost::string_view fullPinyin,
                 boost::string_view hanzi, float cost = 0.0f);

    FCITX_DECLARE_SIGNAL(PinyinDictionary, dictionaryChanged, void(size_t));

protected:
    void
    matchPrefixImpl(const SegmentGraph &graph, GraphMatchCallback callback,
                    const std::unordered_set<const SegmentGraphNode *> &ignore,
                    void *helper) override;

private:
    void loadText(size_t idx, std::istream &in);
    void loadBinary(size_t idx, std::istream &in);

    std::unique_ptr<PinyinDictionaryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(PinyinDictionary);
};
}

#endif // _FCITX_LIBIME_UTF8_PINYINDICTIONARY_H_
