/*
 * SPDX-FileCopyrightText: 2015-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _FCITX_LIBIME_TABLE_TABLEBASEDDICTIONARY_H_
#define _FCITX_LIBIME_TABLE_TABLEBASEDDICTIONARY_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/signals.h>
#include <libime/core/dictionary.h>
#include <libime/core/segmentgraph.h>
#include <libime/table/libimetable_export.h>

namespace libime {
class TableBasedDictionaryPrivate;
class TableOptions;

enum class PhraseFlag {
    None = 1,
    Pinyin,
    Prompt,
    ConstructPhrase,
    User,
    Auto,
    Invalid
};

using TableMatchCallback = std::function<bool(
    std::string_view, std::string_view, uint32_t, PhraseFlag)>;

enum class TableFormat { Text, Binary };
enum class TableMatchMode { Exact, Prefix };

class TableRule;

class LIBIMETABLE_EXPORT TableBasedDictionary
    : public Dictionary,
      public fcitx::ConnectableObject {
    friend class TableContextPrivate;

public:
    TableBasedDictionary();
    virtual ~TableBasedDictionary();

    TableBasedDictionary(const TableBasedDictionary &other) = delete;

    void load(const char *filename, TableFormat format = TableFormat::Binary);
    void load(std::istream &in, TableFormat format = TableFormat::Binary);
    void save(const char *filename, TableFormat format = TableFormat::Binary);
    void save(std::ostream &out, TableFormat format = TableFormat::Binary);

    void loadUser(const char *filename,
                  TableFormat format = TableFormat::Binary);
    void loadUser(std::istream &in, TableFormat format = TableFormat::Binary);
    void saveUser(const char *filename,
                  TableFormat format = TableFormat::Binary);
    void saveUser(std::ostream &out, TableFormat format = TableFormat::Binary);

    size_t loadExtra(const char *filename,
                     TableFormat format = TableFormat::Binary);
    size_t loadExtra(std::istream &in,
                     TableFormat format = TableFormat::Binary);
    void saveExtra(size_t index, const char *filename,
                   TableFormat format = TableFormat::Binary);
    void saveExtra(size_t index, std::ostream &out,
                   TableFormat format = TableFormat::Binary);

    void removeAllExtra();

    bool hasRule() const noexcept;
    bool hasCustomPrompt() const noexcept;
    const TableRule *findRule(std::string_view name) const;
    bool insert(std::string_view key, std::string_view value,
                PhraseFlag flag = PhraseFlag::None,
                bool verifyWithRule = false);
    bool insert(std::string_view value, PhraseFlag flag = PhraseFlag::None);
    bool generate(std::string_view value, std::string &key) const;
    bool generateWithHint(std::string_view value,
                          const std::vector<std::string> &codeHint,
                          std::string &key) const;

    bool isInputCode(uint32_t c) const;
    bool isAllInputCode(std::string_view code) const;
    bool isEndKey(uint32_t c) const;

    bool hasPinyin() const;
    uint32_t maxLength() const;
    bool isValidLength(size_t length) const;

    void statistic() const;

    void setTableOptions(TableOptions option);
    const TableOptions &tableOptions() const;

    bool matchWords(std::string_view code, TableMatchMode mode,
                    const TableMatchCallback &callback) const;

    bool hasMatchingWords(std::string_view code) const;
    bool hasMatchingWords(std::string_view code, std::string_view next) const;

    bool hasOneMatchingWord(std::string_view code) const;

    PhraseFlag wordExists(std::string_view code, std::string_view word) const;
    void removeWord(std::string_view code, std::string_view word);

    std::string reverseLookup(std::string_view word,
                              PhraseFlag flag = PhraseFlag::None) const;
    std::string hint(std::string_view key) const;

    FCITX_DECLARE_SIGNAL(TableBasedDictionary, tableOptionsChanged, void());

private:
    void loadText(std::istream &in);
    void loadBinary(std::istream &in);
    void saveText(std::ostream &out);
    void saveBinary(std::ostream &origOut);

    void
    matchPrefixImpl(const SegmentGraph &graph,
                    const GraphMatchCallback &callback,
                    const std::unordered_set<const SegmentGraphNode *> &ignore,
                    void *helper) const override;

    std::unique_ptr<TableBasedDictionaryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TableBasedDictionary);
};
} // namespace libime

#endif // _FCITX_LIBIME_TABLE_TABLEBASEDDICTIONARY_H_
