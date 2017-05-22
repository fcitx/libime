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
#ifndef _FCITX_LIBIME_LANGUAGEMODEL_H_
#define _FCITX_LIBIME_LANGUAGEMODEL_H_

#include "libime_export.h"
#include <boost/utility/string_view.hpp>
#include <fcitx-utils/macros.h>
#include <memory>
#include <vector>

namespace libime {

using WordIndex = unsigned int;
constexpr const unsigned int InvalidWordIndex =
    std::numeric_limits<WordIndex>::max();
constexpr size_t StateSize = 20 + sizeof(void *);
using State = std::array<char, StateSize>;

class WordNode;
class LatticeNode;
class LanguageModelPrivate;

class LIBIME_EXPORT LanguageModelBase {
public:
    virtual ~LanguageModelBase() {}

    virtual WordIndex beginSentence() const = 0;
    virtual WordIndex endSentence() const = 0;
    virtual WordIndex unknown() const = 0;
    virtual const State &beginState() const = 0;
    virtual const State &nullState() const = 0;
    virtual WordIndex index(boost::string_view view) const = 0;
    virtual float score(const State &state, const WordNode &word,
                        State &out) const = 0;
    bool isNodeUnknown(const LatticeNode &node) const;
    virtual bool isUnknown(WordIndex idx, boost::string_view view) const = 0;
};

class StaticLanguageModelFilePrivate;

class LIBIME_EXPORT StaticLanguageModelFile {
    friend class LanguageModelPrivate;

public:
    explicit StaticLanguageModelFile(const char *file);
    virtual ~StaticLanguageModelFile();

private:
    std::unique_ptr<StaticLanguageModelFilePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(StaticLanguageModelFile);
};

class LIBIME_EXPORT LanguageModel : public LanguageModelBase {
public:
    LanguageModel(const char *file);
    LanguageModel(std::shared_ptr<StaticLanguageModelFile> file);
    virtual ~LanguageModel();

    WordIndex beginSentence() const override;
    WordIndex endSentence() const override;
    WordIndex unknown() const override;
    const State &beginState() const override;
    const State &nullState() const override;
    WordIndex index(boost::string_view view) const override;
    float score(const State &state, const WordNode &word,
                State &out) const override;
    bool isUnknown(WordIndex idx, boost::string_view view) const override;
    void setUnknownPenalty(float unknown);
    float unknownPenalty() const;

private:
    std::unique_ptr<LanguageModelPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(LanguageModel);
};
}

#endif // _FCITX_LIBIME_LANGUAGEMODEL_H_
