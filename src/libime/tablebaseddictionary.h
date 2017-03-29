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

#ifndef LIBIME_TABLE_H
#define LIBIME_TABLE_H

#include "libime_export.h"
#include <fcitx-utils/macros.h>
#include <memory>

namespace libime {
class TableBasedDictionaryPrivate;

enum PhraseFlag {
    PhraseFlagNone = 1,
    PhraseFlagPinyin,
    PhraseFlagPrompt,
    PhraseFlagConstructPhrase
};

class LIBIME_EXPORT TableBasedDictionary {
public:
    enum class TableFormat { Text, Binary };

    TableBasedDictionary();
    virtual ~TableBasedDictionary();

    TableBasedDictionary(const TableBasedDictionary &other);
    TableBasedDictionary(TableBasedDictionary &&other) noexcept;
    explicit TableBasedDictionary(const char *filename,
                                  TableFormat format = TableFormat::Binary);
    explicit TableBasedDictionary(std::istream &in,
                                  TableFormat format = TableFormat::Binary);

    TableBasedDictionary &operator=(TableBasedDictionary other);

    friend void swap(TableBasedDictionary &lhs,
                     TableBasedDictionary &rhs) noexcept;

    void dump(const char *filename);
    void dump(std::ostream &out);
    void save(const char *filename);
    void save(std::ostream &out);

    bool hasRule() const noexcept;
    bool insert(const std::string &key, const std::string &value,
                libime::PhraseFlag flag = PhraseFlagNone,
                bool verifyWithRule = false);
    bool insert(const std::string &value);
    bool generate(const std::string &value, std::string &key);

    void statistic();

private:
    void build(std::istream &in);
    void open(std::istream &in);

    std::unique_ptr<TableBasedDictionaryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TableBasedDictionary);
};

void swap(TableBasedDictionary &lhs, TableBasedDictionary &rhs) noexcept;
}

#endif // LIBIME_TABLE_H
