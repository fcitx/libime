/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_PINYIN_PREDICTION_H_
#define _FCITX_LIBIME_PINYIN_PREDICTION_H_

#include <libime/core/prediction.h>
#include "libime/pinyin/pinyindictionary.h"
#include "libimepinyin_export.h"

namespace libime {

class PinyinPredictionPrivate;

enum class PinyinPredictionSource {
    Model,
    Dictionary
};

/**
 * This is a prediction class that allows to predict against both model and pinyin dictionary.
 */
class LIBIMEPINYIN_EXPORT PinyinPrediction : public Prediction {
public:
    PinyinPrediction();
    virtual ~PinyinPrediction();

    /**
     * Set the pinyin dictionary used for prediction.
     */
    void setPinyinDictionary(const PinyinDictionary *dict);

    /**
     * Predict from model and pinyin dictionary for the last sentnce being type.
     */
    std::vector<std::pair<std::string, PinyinPredictionSource>>
    predict(const State &state, const std::vector<std::string> &sentence, std::string_view lastEncodedPinyin,
            size_t maxSize = 0);

    /**
     * Same as the Prediction::predict with the same signature.
     */
    std::vector<std::string>
    predict(const std::vector<std::string> &sentence = {}, size_t maxSize = 0);

private:
    std::unique_ptr<PinyinPredictionPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(PinyinPrediction);
};

} // namespace libime

#endif // _LIBIM_LIBIME_CORE_PREDICTION_H_
