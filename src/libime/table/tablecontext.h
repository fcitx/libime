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
#ifndef _FCITX_LIBIME_TABLE_TABLECONTEXT_H_
#define _FCITX_LIBIME_TABLE_TABLECONTEXT_H_

/// \file
/// \brief Class provide input method support for table-based ones, like wubi.

#include "libimetable_export.h"
#include <boost/utility/string_view.hpp>
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/macros.h>
#include <libime/core/inputbuffer.h>
#include <libime/core/lattice.h>
#include <tuple>

namespace libime {

class TableContextPrivate;
class TableBasedDictionary;
class UserLanguageModel;

/// \brief Input context for table input method.
class LIBIMETABLE_EXPORT TableContext : public InputBuffer {
public:
    typedef boost::any_range<const SentenceResult,
                             boost::random_access_traversal_tag>
        CandidateRange;

    TableContext(TableBasedDictionary &dict, UserLanguageModel &model);
    virtual ~TableContext();

    void erase(size_t from, size_t to) override;

    void select(size_t idx);

    bool isValidInput(uint32_t c) const;

    CandidateRange candidates() const;

    std::string candidateHint(size_t idx, bool custom = false) const;

    bool selected() const;
    size_t selectedSize() const;
    std::tuple<std::string, bool> selectedSegment(size_t idx) const;
    size_t selectedSegmentLength(size_t idx) const;

    /// \brief A simple preedit implementation.
    /// The value is derived from function selectedSegment and currentCode.
    std::string preedit() const;

    /// \brief Current unselected code.
    const std::string &currentCode() const;

    /// \brief The concatenation of all selectedSegment where bool == true.
    std::string selectedSentence() const;
    size_t selectedLength() const;

    /// \brief Save the current selected text.
    void learn();

    /// \brief Learn auto word from string.
    ///
    /// Depending on the tableOptions, it will try to learn the word in history.
    void learnAutoPhrase(boost::string_view history);

    const TableBasedDictionary &dict() const;
    TableBasedDictionary &mutableDict();

    const UserLanguageModel &model() const;
    UserLanguageModel &mutableModel();
    void autoSelect();

protected:
    void typeImpl(const char *s, size_t length) override;

private:
    void update();
    void typeOneChar(boost::string_view chr);

    std::unique_ptr<TableContextPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TableContext);
};
}

#endif // _FCITX_LIBIME_TABLE_TABLECONTEXT_H_
