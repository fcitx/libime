/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_TABLE_AUTOPHRASEDICT_H_
#define _FCITX_LIBIME_TABLE_AUTOPHRASEDICT_H_

#include "libimetable_export.h"
#include <cstddef>
#include <fcitx-utils/macros.h>
#include <functional>
#include <istream>
#include <memory>
#include <string>
#include <string_view>

namespace libime {

class AutoPhraseDictPrivate;

/// \brief A simple MRU based dictionary.
class LIBIMETABLE_EXPORT AutoPhraseDict {
public:
    AutoPhraseDict(size_t maxItems);
    AutoPhraseDict(size_t maxItems, std::istream &in);
    FCITX_DECLARE_VIRTUAL_DTOR_COPY_AND_MOVE(AutoPhraseDict)

    /// \brief Insert a word into dictionary and refresh the MRU.
    ///
    /// Set the value of entry to value if value is positive. Otherwise if the
    /// value is 0, the actual value will be increased the value by 1.
    void insert(const std::string &entry, uint32_t value = 0);

    /// \brief Check if any word starting with s exists in the dictionary.
    bool search(
        std::string_view s,
        const std::function<bool(std::string_view, uint32_t)> &callback) const;

    /// \brief Returns 0 if there is no such word.
    uint32_t exactSearch(std::string_view s) const;
    void erase(std::string_view s);
    void clear();

    void load(std::istream &in);
    void save(std::ostream &out);

    bool empty() const;

private:
    std::unique_ptr<AutoPhraseDictPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(AutoPhraseDict);
};
} // namespace libime

#endif // _FCITX_LIBIME_TABLE_AUTOPHRASEDICT_H_
