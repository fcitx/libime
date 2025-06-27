/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <algorithm>
#include <string>
#include <fcitx-utils/log.h>
#include "libime/core/userlanguagemodel.h"
#include "libime/pinyin/pinyindictionary.h"
#include "libime/pinyin/pinyinencoder.h"
#include "libime/pinyin/pinyinprediction.h"
#include "testdir.h"

using namespace libime;

namespace fcitx {

LogMessageBuilder &operator<<(LogMessageBuilder &log,
                              PinyinPredictionSource type) {
    switch (type) {
    case PinyinPredictionSource::Dictionary:
        log << "Dict";
        break;
    case PinyinPredictionSource::Model:
        log << "Model";
        break;
    }
    return log;
}

} // namespace fcitx

int main() {
    UserLanguageModel model(LIBIME_BINARY_DIR "/data/sc.lm");
    PinyinDictionary dict;
    dict.load(PinyinDictionary::SystemDict,
              LIBIME_BINARY_DIR "/data/dict_sc.txt", PinyinDictFormat::Text);

    PinyinPrediction prediction;
    prediction.setUserLanguageModel(&model);
    prediction.setPinyinDictionary(&dict);
    auto py = PinyinEncoder::encodeFullPinyin("zhong'guo");
    auto result = prediction.predict(model.nullState(), {"我", "喜欢", "中国"},
                                     {py.data(), py.size()}, 20);
    auto noPyResult = prediction.predict({"我", "喜欢", "中国"}, 20);
    FCITX_ASSERT(result.size() > noPyResult.size())
        << result << " " << noPyResult;

    // Check if word that exists in multiple sub dicts won't generate multiple
    // result.
    py = PinyinEncoder::encodeFullPinyin("guan'xi");
    FCITX_ASSERT(
        dict.lookupWord(PinyinDictionary::SystemDict, "guan'xi'ren", "关系人"));
    dict.addWord(PinyinDictionary::UserDict, "guan'xi'ren", "关系人");
    result = prediction.predict(model.nullState(), {"关系"},
                                {py.data(), py.size()}, 49);
    FCITX_ASSERT(
        std::count_if(result.begin(), result.end(), [](const auto &item) {
            return std::get<std::string>(item) == "人";
        }) == 1);
    return 0;
}
