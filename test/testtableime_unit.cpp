/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
    options.setLearning(true);
    options.setAutoPhraseLength(-1);
    options.setAutoSelect(true);
    options.setAutoSelectLength(-1);
    options.setNoMatchAutoSelectLength(-1);
    options.setNoSortInputLength(2);
    options.setAutoRuleSet({});
    options.setMatchingKey('z');
    options.setOrderPolicy(OrderPolicy::Freq);
    dict.setTableOptions(options);
    TableContext c(dict, model);

    // This candidate does not exist in table, should not be selected.
    c.type("qfgo");
    FCITX_ASSERT(!c.selected());

    c.clear();
    c.type("qfgop");
    FCITX_ASSERT(!c.selected());
    c.clear();

    c.type("vb");

    for (const auto &candidate : c.candidates()) {
        FCITX_INFO() << candidate.toString() << candidate.score();
    }
    c.select(0);
    c.learn();
    c.clear();

    c.type("bbh");

    for (const auto &candidate : c.candidates()) {
        FCITX_INFO() << candidate.toString() << candidate.score();
    }
    c.select(0);
    c.learn();
    c.clear();
    c.learnAutoPhrase("好耶");
    FCITX_ASSERT(c.dict().wordExists("vbbb", "好耶") ==
                 libime::PhraseFlag::Auto);
    c.learnAutoPhrase("好耶", {"ky", "cy"});
    FCITX_ASSERT(c.dict().wordExists("kycy", "好耶") ==
                 libime::PhraseFlag::Auto);

    for (int i = 0; i < 2; i++) {
        c.type("vbbb");

        for (const auto &candidate : c.candidates()) {
            FCITX_INFO() << candidate.toString() << candidate.score();
        }
        c.select(1);
        c.learn();
        c.clear();
        FCITX_INFO() << "========================";
    }

    return 0;
}
