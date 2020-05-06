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

#include "libime/core/userlanguagemodel.h"
#include "libime/table/tablebaseddictionary.h"
#include "libime/table/tablecontext.h"
#include "libime/table/tabledecoder.h"
#include "libime/table/tableoptions.h"
#include "testdir.h"
#include "testutils.h"
#include <fcitx-utils/log.h>

using namespace libime;

class TestLmResolver : public LanguageModelResolver {
public:
    TestLmResolver(std::string_view path) : path_(path) {}

protected:
    std::string
    languageModelFileNameForLanguage(const std::string &language) override {
        if (language == "zh_CN") {
            return path_;
        }
        return {};
    }

private:
    std::string path_;
};

int main() {
    fcitx::Log::setLogRule("*=5");
    TestLmResolver lmresolver(LIBIME_BINARY_DIR "/data/sc.lm");
    auto lm = lmresolver.languageModelFileForLanguage("zh_CN");
    TableBasedDictionary dict;
    UserLanguageModel model(lm);
    dict.load(LIBIME_BINARY_DIR "/data/wbx.main.dict");
    TableOptions options;
    options.setLanguageCode("zh_CN");
    options.setAutoSelect(true);
    options.setAutoSelectLength(-1);
    options.setNoMatchAutoSelectLength(-1);
    options.setNoSortInputLength(2);
    options.setAutoRuleSet({"e2"});
    options.setMatchingKey('z');
    options.setOrderPolicy(OrderPolicy::Freq);
    dict.setTableOptions(options);
    TableContext c(dict, model);

    // This candidate does not exist in table, should not be selected.
    c.type("qfgo");
    FCITX_ASSERT(!c.selected());

    return 0;
}
