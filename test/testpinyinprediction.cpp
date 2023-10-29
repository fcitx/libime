/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/userlanguagemodel.h"
#include "libime/pinyin/pinyindictionary.h"
#include "libime/pinyin/pinyinencoder.h"
#include "libime/pinyin/pinyinprediction.h"
#include "testdir.h"
#include "testutils.h"
#include <fcitx-utils/log.h>
#include <sstream>

using namespace libime;

namespace fcitx {

LogMessageBuilder &operator<<(LogMessageBuilder& log, PinyinPredictionSource type) {
    switch(type) {
        case PinyinPredictionSource::Dictionary:
        log << "Dict";
        break;
        case PinyinPredictionSource::Model:
        log << "Model";
        break;
    }
    return log;
}

}

int main() {
    UserLanguageModel model(LIBIME_BINARY_DIR "/data/sc.lm");
    PinyinDictionary dict;
    dict.load(PinyinDictionary::SystemDict,
              LIBIME_BINARY_DIR "/data/dict_sc.txt", PinyinDictFormat::Text);

    PinyinPrediction prediction;
    prediction.setUserLanguageModel(&model);
    prediction.setPinyinDictionary(&dict);
    auto py = PinyinEncoder::encodeFullPinyin("zhong'guo");
    auto result = prediction.predict(model.nullState(), {"我", "喜欢", "中国"}, {py.data(), py.size()}, 20);
    FCITX_INFO() << result;
    FCITX_INFO() << prediction.predict({"我", "喜欢", "中国"}, 20);
    return 0;
}
