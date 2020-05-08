/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "libime/core/historybigram.h"
#include "libime/core/languagemodel.h"
#include "libime/core/prediction.h"
#include "testdir.h"
#include "testutils.h"
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <fcitx-utils/log.h>
#include <fcitx-utils/stringutils.h>
#include <fstream>
#include <functional>

using namespace libime;

int main() {
    UserLanguageModel model(LIBIME_BINARY_DIR "/data/sc.lm");
    Prediction pred;
    pred.setUserLanguageModel(&model);
    model.history().add({"你", "希望"});
    for (auto result : pred.predict(std::vector<std::string>{"你"})) {
        FCITX_LOG(Info) << result;
    }
    for (auto result : pred.predict(std::vector<std::string>{"你"})) {
        FCITX_LOG(Info) << result;
    }

    return 0;
}
