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
