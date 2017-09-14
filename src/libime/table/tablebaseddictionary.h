/*
 * Project libime
 * Copyright (c) Xuetian Weng 2015, All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */

#ifndef _FCITX_LIBIME_TABLE_TABLEBASEDDICTIONARY_H_
#define _FCITX_LIBIME_TABLE_TABLEBASEDDICTIONARY_H_

#include "libimetable_export.h"
#include <fcitx-utils/macros.h>
#include <fcitx-utils/signals.h>
#include <libime/core/dictionary.h>
#include <memory>

namespace libime {
class TableBasedDictionaryPrivate;
class TableOptions;

enum PhraseFlag {
    PhraseFlagNone = 1,
    PhraseFlagPinyin,
    PhraseFlagPrompt,
    PhraseFlagConstructPhrase
};

typedef std::function<bool(boost::string_view code, boost::string_view word,
                           float cost)>
    TableMatchCallback;

enum class TableFormat { Text, Binary };
enum class TableMatchMode { Exact, Prefix };

class TableRule;

class LIBIMETABLE_EXPORT TableBasedDictionary
    : public Dictionary,
      public fcitx::ConnectableObject {
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

    bool hasRule() const noexcept;
    const TableRule *findRule(boost::string_view name) const;
    bool insert(boost::string_view key, boost::string_view value,
                libime::PhraseFlag flag = PhraseFlagNone,
                bool verifyWithRule = false);
    bool insert(boost::string_view value);
    bool generate(boost::string_view value, std::string &key);

    bool isInputCode(uint32_t c) const;
    bool isAllInputCode(boost::string_view code) const;
    bool isEndKey(uint32_t c) const;

    bool hasPinyin() const;
    int32_t maxLength() const;
    bool isValidLength(size_t length) const;
    int32_t pinyinLength() const;

    void statistic() const;

    void setTableOptions(TableOptions option);
    const TableOptions &tableOptions() const;

    void matchWords(boost::string_view code, TableMatchMode mode,
                    const TableMatchCallback &callback) const;

    bool hasMatchingWords(boost::string_view code) const;
    bool hasMatchingWords(boost::string_view code,
                          boost::string_view next) const;

    bool hasOneMatchingWord(boost::string_view code) const;

    std::string
    reverseLookup(boost::string_view word,
                  PhraseFlag flag = PhraseFlag::PhraseFlagNone) const;

    FCITX_DECLARE_SIGNAL(TableBasedDictionary, tableOptionsChanged, void());

private:
    void loadText(std::istream &in);
    void loadBinary(std::istream &in);
    void saveText(std::ostream &out);
    void saveBinary(std::ostream &out);
    void parseDataLine(boost::string_view buf);

    void
    matchPrefixImpl(const SegmentGraph &graph,
                    const GraphMatchCallback &callback,
                    const std::unordered_set<const SegmentGraphNode *> &ignore,
                    void *helper) const override;

    std::unique_ptr<TableBasedDictionaryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TableBasedDictionary);
};
}

#endif // _FCITX_LIBIME_TABLE_TABLEBASEDDICTIONARY_H_
