/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_HISTORYBIGRAM_H_
#define _FCITX_LIBIME_CORE_HISTORYBIGRAM_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>
#include <fcitx-utils/macros.h>
#include <libime/core/lattice.h>
#include <libime/core/libimecore_export.h>

namespace libime {

class HistoryBigramPrivate;

using ValidationCodeExtractor = std::function<std::string(const WordNode *)>;

class LIBIMECORE_EXPORT HistoryBigram {
public:
    using WordWithCode = std::pair<std::string, std::string>;
    using WordWithCodeView = std::pair<std::string_view, std::string_view>;

    HistoryBigram();

    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(HistoryBigram);

    void load(std::istream &in);
    void loadText(std::istream &in);
    void save(std::ostream &out);
    void dump(std::ostream &out);
    void clear();

    /// Set unknown probability penatly.
    /// \param unknown is a log probability.
    void setUnknownPenalty(float unknown);
    float unknownPenalty() const;

    void setUseOnlyUnigram(bool useOnlyUnigram);
    bool useOnlyUnigram() const;

    void forget(std::string_view word);
    void forget(std::string_view word, std::string_view code);

    bool isUnknown(std::string_view v) const;
    float score(const WordNode *prev, const WordNode *cur) const;
    float score(std::string_view prev, std::string_view cur) const;
    float scoreWithCode(WordWithCodeView prev, WordWithCodeView cur) const;
    float scoreWithCode(const WordNode *prev, const WordNode *cur,
                        const ValidationCodeExtractor &extractor) const;
    void add(const SentenceResult &sentence);
    void add(const std::vector<std::string> &sentence);
    void addWithCode(const SentenceResult &sentence,
                     const ValidationCodeExtractor &validationCodeExtractor);
    void
    addWithCode(const std::vector<WordWithCode> &sentenceWithValidationCode);

    /// Fill the prediction based on current sentence.
    void fillPredict(std::unordered_set<std::string> &words,
                     const std::vector<std::string> &sentence,
                     size_t maxSize) const;

    bool containsBigram(std::string_view prev, std::string_view cur) const;

    /**
     * Query the weighted frequency of the unigram.
     *
     * @since 1.1.14
     */
    float unigramFrequency(WordWithCodeView word) const;

    /**
     * Query the weighted frequency of the bigram.
     *
     * @since 1.1.14
     */
    float bigramFrequency(WordWithCodeView prev, WordWithCodeView cur) const;

    /**
     * Query the raw frequency of the unigram.
     *
     * @since 1.1.14
     */
    int32_t rawUnigramFrequency(WordWithCodeView word) const;

    /**
     * Query the raw frequency of the bigram.
     *
     * @since 1.1.14
     */
    int32_t rawBigramFrequency(WordWithCodeView prev,
                               WordWithCodeView cur) const;

    void addWithContext(const std::vector<WordWithCode> &context,
                        std::vector<WordWithCode> newSentence);

private:
    std::unique_ptr<HistoryBigramPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(HistoryBigram);
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_HISTORYBIGRAM_H_
