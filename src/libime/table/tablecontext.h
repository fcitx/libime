/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_TABLE_TABLECONTEXT_H_
#define _FCITX_LIBIME_TABLE_TABLECONTEXT_H_

/// \file
/// \brief Class provide input method support for table-based ones, like wubi.

#include "libimetable_export.h"
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/macros.h>
#include <libime/core/inputbuffer.h>
#include <libime/core/lattice.h>
#include <libime/table/tablebaseddictionary.h>
#include <string_view>
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

    static std::string code(const SentenceResult &sentence);
    static PhraseFlag flag(const SentenceResult &sentence);
    static bool isPinyin(const SentenceResult &sentence);
    static bool isAuto(const SentenceResult &sentence);

    bool selected() const;
    size_t selectedSize() const;
    std::tuple<std::string, bool> selectedSegment(size_t idx) const;
    std::string selectedCode(size_t idx) const;
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

    /// \brief Save the last selected text.
    void learnLast();

    /// \brief Learn auto word from string.
    ///
    /// Depending on the tableOptions, it will try to learn the word in history.
    void learnAutoPhrase(std::string_view history);

    /// \brief Learn auto word from string
    ///
    /// Similar to its overload, but with hint of given code.
    void learnAutoPhrase(std::string_view history,
                         const std::vector<std::string> &hints);

    const TableBasedDictionary &dict() const;
    TableBasedDictionary &mutableDict();

    const UserLanguageModel &model() const;
    UserLanguageModel &mutableModel();
    void autoSelect();

protected:
    bool typeImpl(const char *s, size_t length) override;

private:
    void update();
    bool typeOneChar(std::string_view chr);

    std::unique_ptr<TableContextPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TableContext);
};
} // namespace libime

#endif // _FCITX_LIBIME_TABLE_TABLECONTEXT_H_
