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

    void setPenaltyFactor(float factor);
    float penaltyFactor() const;

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

    void fillPredict(std::unordered_set<std::string> &words,
                     const std::vector<std::string> &sentence,
                     size_t maxSize) const;

private:
    std::unique_ptr<HistoryBigramPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(HistoryBigram);
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_HISTORYBIGRAM_H_
