/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_LANGUAGEMODEL_H_
#define _FCITX_LIBIME_CORE_LANGUAGEMODEL_H_

#include "libimecore_export.h"
#include <array>
#include <cstddef>
#include <fcitx-utils/macros.h>
#include <libime/core/datrie.h>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
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
class LanguageModelResolverPrivate;

class LIBIMECORE_EXPORT LanguageModelBase {
public:
    virtual ~LanguageModelBase() {}

    virtual WordIndex beginSentence() const = 0;
    virtual WordIndex endSentence() const = 0;
    virtual WordIndex unknown() const = 0;
    virtual const State &beginState() const = 0;
    virtual const State &nullState() const = 0;
    virtual WordIndex index(std::string_view view) const = 0;
    virtual float score(const State &state, const WordNode &word,
                        State &out) const = 0;
    virtual bool isUnknown(WordIndex idx, std::string_view view) const = 0;
    bool isNodeUnknown(const LatticeNode &node) const;
    float singleWordScore(std::string_view word) const;
    float singleWordScore(const State &state, std::string_view word) const;
    float wordsScore(const State &state,
                     const std::vector<std::string_view> &word) const;
};

class StaticLanguageModelFilePrivate;

class LIBIMECORE_EXPORT StaticLanguageModelFile {
    friend class LanguageModelPrivate;

public:
    explicit StaticLanguageModelFile(const char *file);
    virtual ~StaticLanguageModelFile();

    const DATrie<float> &predictionTrie() const;

private:
    std::unique_ptr<StaticLanguageModelFilePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(StaticLanguageModelFile);
};

class LIBIMECORE_EXPORT LanguageModel : public LanguageModelBase {
public:
    explicit LanguageModel(const char *file);
    LanguageModel(
        std::shared_ptr<const StaticLanguageModelFile> file = nullptr);
    virtual ~LanguageModel();

    std::shared_ptr<const StaticLanguageModelFile> languageModelFile() const;

    WordIndex beginSentence() const override;
    WordIndex endSentence() const override;
    WordIndex unknown() const override;
    const State &beginState() const override;
    const State &nullState() const override;
    WordIndex index(std::string_view word) const override;
    float score(const State &state, const WordNode &node,
                State &out) const override;
    bool isUnknown(WordIndex idx, std::string_view word) const override;
    void setUnknownPenalty(float unknown);
    float unknownPenalty() const;

private:
    std::unique_ptr<LanguageModelPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(LanguageModel);
};

/// \brief a class that provides language model data for different languages.
///
/// The resolver will also hold a weak reference to the language model file.
/// If the language model file is still alive no new file will be constructed.
class LIBIMECORE_EXPORT LanguageModelResolver {
public:
    LanguageModelResolver();
    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(LanguageModelResolver)
    std::shared_ptr<const StaticLanguageModelFile>
    languageModelFileForLanguage(const std::string &language);

protected:
    virtual std::string
    languageModelFileNameForLanguage(const std::string &language) = 0;

private:
    std::unique_ptr<LanguageModelResolverPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(LanguageModelResolver);
};

class LIBIMECORE_EXPORT DefaultLanguageModelResolver
    : public LanguageModelResolver {
public:
    static DefaultLanguageModelResolver &instance();

protected:
    std::string
    languageModelFileNameForLanguage(const std::string &language) override;

private:
    DefaultLanguageModelResolver();
    ~DefaultLanguageModelResolver();
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_LANGUAGEMODEL_H_
