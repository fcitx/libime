/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/historybigram.h"
#include "libime/core/prediction.h"
#include "testdir.h"
#include <fcitx-utils/log.h>

using namespace libime;

int main() {
    UserLanguageModel model(LIBIME_BINARY_DIR "/data/sc.lm");
    Prediction pred;
    pred.setUserLanguageModel(&model);
    model.history().add({"你", "希望"});
    for (const auto &result : pred.predict(std::vector<std::string>{"你"})) {
        FCITX_LOG(Info) << result;
    }
    for (const auto &result : pred.predict(std::vector<std::string>{"你"})) {
        FCITX_LOG(Info) << result;
    }

    return 0;
}
