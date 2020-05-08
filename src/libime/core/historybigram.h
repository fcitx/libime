/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_HISTORYBIGRAM_H_
#define _FCITX_LIBIME_CORE_HISTORYBIGRAM_H_

#include "libimecore_export.h"
#include <fcitx-utils/macros.h>
#include <libime/core/lattice.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace libime {

class HistoryBigramPrivate;
class HistoryBigram;

class LIBIMECORE_EXPORT HistoryBigram {
public:
    HistoryBigram();

    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(HistoryBigram);

    void load(std::istream &in);
    void save(std::ostream &out);
    void dump(std::ostream &out);
    void clear();

    /// Set unknown probability penatly.
    /// \param unknown is a log probability.
    void setUnknownPenalty(float unknown);
    float unknownPenalty() const;

    void forget(std::string_view word);

    bool isUnknown(std::string_view v) const;
    float score(const WordNode *prev, const WordNode *cur) const {
        return score(prev ? prev->word() : "", cur ? cur->word() : "");
    }
    float score(std::string_view prev, std::string_view cur) const;
    void add(const SentenceResult &sentence);
    void add(const std::vector<std::string> &sentence);

    /// Fill the prediction based on current sentence.
    void fillPredict(std::unordered_set<std::string> &words,
                     const std::vector<std::string> &sentence,
                     size_t maxSize) const;

private:
    std::unique_ptr<HistoryBigramPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(HistoryBigram);
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_HISTORYBIGRAM_H_
