/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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

enum class PinyinDictFlag { NoFlag = 0, FullMatch = (1 << 1) };

using PinyinDictFlags = fcitx::Flags<PinyinDictFlag>;

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
    bool removeWord(size_t idx, std::string_view fullPinyin,
                    std::string_view hanzi);

    void setFlags(size_t idx, PinyinDictFlags flags);

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
