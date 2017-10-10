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
#ifndef _FCITX_LIBIME_TABLE_AUTOPHRASEDICT_H_
#define _FCITX_LIBIME_TABLE_AUTOPHRASEDICT_H_

#include "libimetable_export.h"
#include <boost/utility/string_view.hpp>
#include <cstddef>
#include <fcitx-utils/macros.h>
#include <functional>
#include <istream>
#include <memory>
#include <ostream>
#include <string>

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
    bool
    search(boost::string_view s,
           std::function<bool(boost::string_view, uint32_t)> callback) const;

    /// \brief Returns 0 if there is no such word.
    uint32_t exactSearch(boost::string_view s) const;
    void erase(boost::string_view s);
    void clear();

    void load(std::istream &in);
    void save(std::ostream &out);

private:
    std::unique_ptr<AutoPhraseDictPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(AutoPhraseDict);
};
}

#endif // _FCITX_LIBIME_TABLE_AUTOPHRASEDICT_H_
