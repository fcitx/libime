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
