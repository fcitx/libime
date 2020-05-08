/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_PREDICTION_H_
#define _FCITX_LIBIME_CORE_PREDICTION_H_

#include "libime/core/languagemodel.h"
#include "libime/core/userlanguagemodel.h"
#include "libimecore_export.h"
#include <fcitx-utils/macros.h>
#include <memory>

namespace libime {

class PredictionPrivate;
class HistoryBigram;

class LIBIMECORE_EXPORT Prediction {
public:
    Prediction();
    virtual ~Prediction();

    void setUserLanguageModel(const UserLanguageModel *lm) {
        setLanguageModel(lm);
        setHistoryBigram(&lm->history());
    }
    void setLanguageModel(const LanguageModel *lm);
    void setHistoryBigram(const HistoryBigram *bigram);
    std::vector<std::string>
    predict(const State &state, const std::vector<std::string> &sentence = {},
            size_t maxSize = -1);
    std::vector<std::string>
    predict(const std::vector<std::string> &sentence = {}, size_t maxSize = -1);

private:
    std::unique_ptr<PredictionPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Prediction);
};

} // namespace libime

#endif // _LIBIM_LIBIME_CORE_PREDICTION_H_
