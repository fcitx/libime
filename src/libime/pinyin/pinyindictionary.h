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
#ifndef _FCITX_LIBIME_PINYIN_PINYINDICTIONARY_H_
#define _FCITX_LIBIME_PINYIN_PINYINDICTIONARY_H_

#include "libimepinyin_export.h"
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/macros.h>
#include <libime/core/triedictionary.h>
#include <libime/pinyin/pinyinencoder.h>
#include <memory>

namespace libime {

enum class PinyinDictFormat { Text, Binary };

class PinyinDictionaryPrivate;

typedef std::function<bool(std::string_view encodedPinyin,
                           std::string_view hanzi, float cost)>
    PinyinMatchCallback;
class PinyinDictionary;

using PinyinTrie = typename TrieDictionary::TrieType;

class LIBIMEPINYIN_EXPORT PinyinDictionary : public TrieDictionary {
public:
    explicit PinyinDictionary();
    ~PinyinDictionary();

    // Load dicitonary for a specific dict.
    void load(size_t idx, std::istream &in, PinyinDictFormat format);
    void load(size_t idx, const char *filename, PinyinDictFormat format);

    // Match the word by encoded pinyin.
    void matchWords(const char *data, size_t size,
                    PinyinMatchCallback callback) const;

    void save(size_t idx, const char *filename, PinyinDictFormat format);
    void save(size_t idx, std::ostream &out, PinyinDictFormat format);

    void addWord(size_t idx, std::string_view fullPinyin,
                 std::string_view hanzi, float cost = 0.0f);

    using dictionaryChanged = TrieDictionary::dictionaryChanged;

protected:
    void
    matchPrefixImpl(const SegmentGraph &graph,
                    const GraphMatchCallback &callback,
                    const std::unordered_set<const SegmentGraphNode *> &ignore,
                    void *helper) const override;

private:
    void loadText(size_t idx, std::istream &in);
    void loadBinary(size_t idx, std::istream &in);
    void saveText(size_t idx, std::ostream &out);

    std::unique_ptr<PinyinDictionaryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(PinyinDictionary);
};
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINDICTIONARY_H_
