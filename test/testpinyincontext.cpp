/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <vector>
#include <fcitx-utils/log.h>
#include "libime/core/historybigram.h"
#include "libime/core/lattice.h"
#include "libime/core/userlanguagemodel.h"
#include "libime/pinyin/pinyincontext.h"
#include "libime/pinyin/pinyindecoder.h"
#include "libime/pinyin/pinyindictionary.h"
#include "libime/pinyin/pinyinencoder.h"
#include "libime/pinyin/pinyinime.h"
#include "libime/pinyin/shuangpinprofile.h"
#include "testdir.h"

using namespace libime;

namespace {

void checkCandidateSet(const PinyinContext &context) {
    std::unordered_set<std::string> candidates;
    for (const auto &candidate : context.candidates()) {
        candidates.insert(candidate.toString());
    }

    FCITX_ASSERT(candidates == context.candidateSet());
}

void checkCandidatesToCursorSet(const PinyinContext &context) {
    std::unordered_set<std::string> candidates;
    for (const auto &candidate : context.candidatesToCursor()) {
        candidates.insert(candidate.toString());
    }

    FCITX_ASSERT(candidates == context.candidatesToCursorSet());
}

bool anyCandidatePinyinStartsWith(const PinyinContext &context,
                                  std::string_view prefix) {
    for (const auto &candidate : context.candidates()) {
        if (context.candidateFullPinyin(candidate).starts_with(prefix)) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    PinyinIME ime(
        std::make_unique<PinyinDictionary>(),
        std::make_unique<UserLanguageModel>(LIBIME_BINARY_DIR "/data/sc.lm"));
    ime.model()->history().add({"字迹", "格子"});
    // add a manual dict
    std::stringstream ss;
    ss << "献世 xian'shi 0.0\n";
    ime.dict()->load(PinyinDictionary::SystemDict,
                     LIBIME_BINARY_DIR "/data/sc.dict",
                     PinyinDictFormat::Binary);
    ime.dict()->load(PinyinDictionary::UserDict, ss, PinyinDictFormat::Text);
    ime.dict()->addWord(1, "zi'ji'ge'zi", "自机各自");
    ime.setFuzzyFlags(PinyinFuzzyFlag::Inner);
    PinyinContext c(&ime);

    {
        c.type("xian");
        std::unordered_set<size_t> xiEndIndexes;
        for (const auto &candidate : c.candidates()) {
            if (candidate.toString() == "洗") {
                xiEndIndexes.insert(candidate.sentence().back()->to()->index());
            }
        }
        FCITX_ASSERT(xiEndIndexes.size() == 2) << xiEndIndexes.size();
        FCITX_ASSERT(c.candidateSet().count("洗") == 1);
        c.clear();
    }

    c.type("xianshi");

    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.select(40);
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.cancel();
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.type("shi'");
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    int i = 0;
    for (const auto &candidate : c.candidates()) {
        if (candidate.toString() == "西安市") {
            break;
        }
        i++;
    }
    c.select(i);
    FCITX_ASSERT(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.select(0);
    FCITX_ASSERT(c.selected()) << c.sentence() << " " << c.preedit();
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.clear();
    FCITX_ASSERT(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    c.type("zi'ji'ge'zi'");
    FCITX_ASSERT(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }
    std::cout << "--------------------------------" << std::endl;
    i = 0;
    for (const auto &candidate : c.candidates()) {
        if (candidate.toString() == "子集") {
            break;
        }
        i++;
    }
    c.select(i);
    FCITX_ASSERT(!c.selected());
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    for (const auto &candidate : c.candidates()) {
        std::cout << candidate.toString() << std::endl;
    }

    std::cout << "--------------------------------" << std::endl;
    c.select(0);
    FCITX_ASSERT(c.selected()) << c.sentence() << " " << c.preedit();
    std::cout << c.sentence() << std::endl;
    std::cout << c.preedit() << std::endl;
    c.clear();
    c.type("n");
    for (const auto &candidate : c.candidates()) {
        for (const auto *node : candidate.sentence()) {
            const auto &pinyin = node->as<PinyinLatticeNode>().encodedPinyin();
            std::cout << node->word();
            if (!pinyin.empty()) {
                std::cout << " " << PinyinEncoder::decodeFullPinyin(pinyin);
            }
        }
        std::cout << std::endl;
    }

    std::cout << "--------------------------------" << std::endl;
    {
        c.clear();
        c.type("nvedai");
        std::array<size_t, 7> expectedCursor = {0, 1, 3, 4, 6, 7, 8};
        for (size_t i = 0; i <= c.size(); i++) {
            c.setCursor(i);
            size_t actualCursor =
                c.preeditWithCursor(PinyinPreeditMode::Pinyin).second;
            FCITX_ASSERT(actualCursor == expectedCursor[i])
                << "Raw Cursor: " << i << " Cursor: " << actualCursor
                << " Expected: " << expectedCursor[i];
        }
        FCITX_ASSERT(c.preedit(PinyinPreeditMode::Pinyin) == "nüe dai");
    }

    {
        c.clear();
        c.type("xi'an");
        std::array<size_t, 6> expectedCursor = {0, 1, 2, 4, 6, 7};
        for (size_t i = 0; i <= c.size(); i++) {
            c.setCursor(i);
            size_t actualCursor =
                c.preeditWithCursor(PinyinPreeditMode::RawText).second;
            FCITX_ASSERT(actualCursor == expectedCursor[i])
                << "Raw Cursor: " << i << " Cursor: " << actualCursor
                << " Expected: " << expectedCursor[i];
        }
        FCITX_ASSERT(c.preedit(PinyinPreeditMode::RawText) == "xi ' an");
    }

    {
        c.clear();
        c.type("nianglanghang");
        checkCandidatesToCursorSet(c);
        size_t i = 0;
        for (const auto &candidate : c.candidatesToCursor()) {
            if (candidate.toString() == "娘") {
                break;
            }
            i++;
        }
        FCITX_ASSERT(i < c.candidatesToCursor().size());
        c.selectCandidatesToCursor(i);
        i = c.size();
        while (i > 0) {
            --i;
            c.setCursor(i);
            checkCandidatesToCursorSet(c);
        }
    }

    {
        c.clear();
        c.type("hellonihao");
        c.selectCustom(5, "Hello");
        size_t i = 0;
        for (const auto &candidate : c.candidatesToCursor()) {
            if (candidate.toString() == "你") {
                break;
            }
            i++;
        }
        FCITX_ASSERT(i < c.candidatesToCursor().size());
        c.selectCandidatesToCursor(i);

        FCITX_ASSERT(!c.selected());
        c.selectCustom(3, "What");

        FCITX_ASSERT(c.selected());
        FCITX_ASSERT(c.selectedSentence() == "Hello你What");
    }

    {
        c.clear();
        c.type("shounihao");
        auto shouPinyin = PinyinEncoder::encodeFullPinyin("shou");
        c.selectCustom(4, "✋",
                       std::string_view(shouPinyin.data(), shouPinyin.size()));
        size_t i = 0;
        for (const auto &candidate : c.candidatesToCursor()) {
            if (candidate.toString() == "你好") {
                break;
            }
            i++;
        }
        FCITX_ASSERT(i < c.candidatesToCursor().size());
        c.selectCandidatesToCursor(i);

        FCITX_ASSERT(c.selected());
        c.learn();

        c.clear();
        c.type("shounihao");
        FCITX_ASSERT(c.candidatesToCursorSet().count("✋你好") > 0);
    }
    {
        c.clear();
        c.type("er");
        c.selectCustom(2, "");
        FCITX_ASSERT(c.selected());
        FCITX_ASSERT(c.selectedSentence() == "");
        FCITX_ASSERT(c.selectedWords().size() == 1);
        FCITX_ASSERT(c.selectedWords().front().empty());
        FCITX_ASSERT(c.selectedWordsWithPinyin().size() == 1);
    }

    // Check that context words can change prediction.
    {
        {
            c.clear();
            c.type("ta");
            size_t i = 0;
            for (const auto &candidate : c.candidatesToCursor()) {
                if (candidate.toString() == "她") {
                    break;
                }
                i++;
            }
            FCITX_ASSERT(i > 0) << i;
        }
        {
            c.clear();
            c.setContextWords({"我", "不", "知道"});
            FCITX_ASSERT(c.contextWords() ==
                         std::vector<std::string>{"不", "知道"});
            c.setContextWords({"谁", "他"});
            FCITX_ASSERT(c.contextWords() ==
                         std::vector<std::string>{"谁", "他"});
            c.appendContextWords({"爱"});
            FCITX_ASSERT(c.contextWords() ==
                         std::vector<std::string>{"他", "爱"});
            c.type("ta");
            size_t i = 0;
            for (const auto &candidate : c.candidatesToCursor()) {
                if (candidate.toString() == "她") {
                    break;
                }
                i++;
            }
            FCITX_ASSERT(i == 0) << i;
        }
    }

    {
        c.clear();
        c.clearContextWords();
        auto wordCandidateLimit = ime.wordCandidateLimit();
        ime.setWordCandidateLimit(1);
        c.type("ziran");
        checkCandidateSet(c);
        ime.setWordCandidateLimit(wordCandidateLimit);
    }

    {
        c.clear();
        c.clearContextWords();
        c.type("ziran");
        checkCandidateSet(c);
        c.clear();
        FCITX_ASSERT(!ime.model()->history().containsBigram("他", "爱"));
        c.type("taai");
        checkCandidateSet(c);
        checkCandidatesToCursorSet(c);
        size_t i = 0;
        for (const auto &candidate : c.candidatesToCursor()) {
            if (candidate.toString() == "他爱") {
                break;
            }
            i++;
        }
        FCITX_ASSERT(i < c.candidatesToCursor().size());
        c.selectCandidatesToCursor(i);

        FCITX_ASSERT(c.selected());
        FCITX_ASSERT(c.selectedSentence() == "他爱");
        c.learn();
        c.clear();
        FCITX_ASSERT(ime.model()->history().containsBigram("他", "爱"));
    }

    // Pinyin fixes can force a syllable and a segmentation.
    {
        ime.dict()->addWord(PinyinDictionary::UserDict, "nu", "怒");
        ime.dict()->addWord(PinyinDictionary::UserDict, "ni", "你");
        ime.dict()->addWord(PinyinDictionary::UserDict, "xian", "线");
        ime.dict()->addWord(PinyinDictionary::UserDict, "xi'an", "西安");

        // Force "xian" to be read as "xi" + "an".
        c.clear();
        c.type("xian");
        checkCandidateSet(c);
        FCITX_ASSERT(c.pinyinFixes().empty());
        // "xian" as a whole is a valid match of 线.
        FCITX_ASSERT(c.candidateSet().count("线") > 0);
        // 西安 crosses the edge but its syllables match the natural parse.
        FCITX_ASSERT(c.candidateSet().count("西安") > 0);

        c.setPinyinFixes({{0, 2, {PinyinInitial::X, PinyinFinal::I}}});
        FCITX_ASSERT(c.pinyinFixes().size() == 1);
        FCITX_ASSERT(c.pinyinFixes().front().start_ == 0);
        FCITX_ASSERT(c.pinyinFixes().front().end_ == 2);
        checkCandidateSet(c);
        // 线 is read as a whole "xian", which crosses the fix.
        FCITX_ASSERT(c.candidateSet().count("线") == 0);
        // 西安 spans the fixed boundary but its syllables match the fix.
        FCITX_ASSERT(c.candidateSet().count("西安") > 0);

        // Clearing the fixes restores everything.
        c.setPinyinFixes({});
        FCITX_ASSERT(c.pinyinFixes().empty());
        checkCandidateSet(c);
        FCITX_ASSERT(c.candidateSet().count("线") > 0);
    }

    // A fix with an invalid final keeps the initial and allows any final.
    {
        c.clear();
        c.type("h");
        checkCandidateSet(c);
        FCITX_ASSERT(!c.candidates().empty());

        c.setPinyinFixes({{0, 1, {PinyinInitial::H, PinyinFinal::Invalid}}});
        checkCandidateSet(c);
        FCITX_ASSERT(!c.candidates().empty());
        FCITX_ASSERT(anyCandidatePinyinStartsWith(c, "h"));

        [[maybe_unused]] auto [preedit, cursor, highlights] =
            c.preeditWithCursorHighlight(PinyinPreeditMode::RawText);
        FCITX_ASSERT(preedit == "h") << preedit;
        FCITX_ASSERT(highlights.size() == 1);
        FCITX_ASSERT(highlights.front() == std::pair<size_t, size_t>(0, 1));

        // The same wildcard reading works for a longer input.
        c.clear();
        c.type("hhh");
        c.setPinyinFixes({{0, 1, {PinyinInitial::H, PinyinFinal::Invalid}}});
        checkCandidateSet(c);
        FCITX_ASSERT(!c.candidates().empty());
        std::tie(preedit, cursor, highlights) =
            c.preeditWithCursorHighlight(PinyinPreeditMode::RawText);
        FCITX_ASSERT(preedit == "h h h") << preedit;
        FCITX_ASSERT(highlights.size() == 1);
        FCITX_ASSERT(highlights.front() == std::pair<size_t, size_t>(0, 1));
    }

    // A fix can force a syllable to be read differently.
    {
        c.clear();
        c.type("nuhaoxian");
        checkCandidateSet(c);
        const auto baseline = c.candidateSet();
        FCITX_ASSERT(!baseline.empty());
        FCITX_ASSERT(anyCandidatePinyinStartsWith(c, "nu"));
        FCITX_ASSERT(!anyCandidatePinyinStartsWith(c, "ni"));

        // Force "nu" to be read as "ni".
        c.setPinyinFixes({{0, 2, {PinyinInitial::N, PinyinFinal::I}}});
        checkCandidateSet(c);
        FCITX_ASSERT(c.candidateSet() != baseline);
        FCITX_ASSERT(!anyCandidatePinyinStartsWith(c, "nu"));
        FCITX_ASSERT(anyCandidatePinyinStartsWith(c, "ni"));
        FCITX_ASSERT(!c.candidates().empty());
        FCITX_ASSERT(
            c.candidateFullPinyin(c.candidates()[0]).starts_with("ni"));

        // Selecting a word makes the fix dormant, cancelling it brings the fix
        // back.
        c.select(0);
        FCITX_ASSERT(c.selectedLength() > 2);
        FCITX_ASSERT(c.pinyinFixes().size() == 1);
        c.cancel();
        FCITX_ASSERT(c.selectedLength() == 0);
        FCITX_ASSERT(c.pinyinFixes().size() == 1);
        FCITX_ASSERT(!c.candidates().empty());
        FCITX_ASSERT(!anyCandidatePinyinStartsWith(c, "nu"));
    }

    // Preedit displays the fixed syllable instead of the raw input.
    {
        ime.dict()->addWord(PinyinDictionary::UserDict, "shui", "水");
        c.clear();
        c.type("nuhaoxian");

        auto [preedit, cursor, highlights] =
            c.preeditWithCursorHighlight(PinyinPreeditMode::RawText);
        FCITX_ASSERT(preedit == "nu hao xian") << preedit;
        FCITX_ASSERT(highlights.empty());

        c.setPinyinFixes({{0, 2, {PinyinInitial::N, PinyinFinal::I}}});
        std::tie(preedit, cursor, highlights) =
            c.preeditWithCursorHighlight(PinyinPreeditMode::RawText);
        FCITX_ASSERT(preedit == "ni hao xian") << preedit;
        FCITX_ASSERT(highlights.size() == 1);
        FCITX_ASSERT(highlights.front() == std::pair<size_t, size_t>(0, 2));

        std::tie(preedit, cursor, highlights) =
            c.preeditWithCursorHighlight(PinyinPreeditMode::Pinyin);
        FCITX_ASSERT(preedit == "ni hao xian") << preedit;
        FCITX_ASSERT(highlights.size() == 1);
        FCITX_ASSERT(highlights.front() == std::pair<size_t, size_t>(0, 2));

        // The overload without an explicit mode uses the IME's mode.
        auto [defaultPreedit, defaultCursor, defaultHighlights] =
            c.preeditWithCursorHighlight();
        FCITX_ASSERT(defaultPreedit == "ni hao xian") << defaultPreedit;
        FCITX_ASSERT(defaultHighlights.size() == 1);
        FCITX_ASSERT(defaultHighlights.front() ==
                     std::pair<size_t, size_t>(0, 2));
        (void)defaultCursor;

        // A fixed syllable can be longer than the raw input. A cursor at the
        // end of the fix maps to the end of the fixed pinyin.
        c.clear();
        c.type("nuhaoxian");
        c.setPinyinFixes({{0, 2, {PinyinInitial::SH, PinyinFinal::UI}}});
        c.setCursor(2);
        std::tie(preedit, cursor, highlights) =
            c.preeditWithCursorHighlight(PinyinPreeditMode::RawText);
        FCITX_ASSERT(preedit == "shui hao xian") << preedit;
        FCITX_ASSERT(cursor == 4) << cursor;
        FCITX_ASSERT(highlights.size() == 1);
        FCITX_ASSERT(highlights.front() == std::pair<size_t, size_t>(0, 4));

        // A selected word makes the fix dormant, so nothing is fixed in the
        // preedit. Cancelling the selection brings the fix back.
        c.setPinyinFixes({{0, 2, {PinyinInitial::N, PinyinFinal::I}}});
        c.select(0);
        std::tie(preedit, cursor, highlights) =
            c.preeditWithCursorHighlight(PinyinPreeditMode::RawText);
        FCITX_ASSERT(highlights.empty());
        c.cancel();
        std::tie(preedit, cursor, highlights) =
            c.preeditWithCursorHighlight(PinyinPreeditMode::RawText);
        FCITX_ASSERT(!highlights.empty());
    }

    // Preedit positions can be mapped back to the input cursor.
    {
        c.clear();
        c.type("xi'an");
        const auto mode = PinyinPreeditMode::RawText;
        auto [preedit, cursor] = c.preeditWithCursor(mode);
        FCITX_ASSERT(preedit == "xi ' an") << preedit;
        (void)cursor;

        // Positions 3 and 5 are gaps caused by the inserted spaces; they
        // snap to the input cursor on their left.
        const std::array<size_t, 9> expected = {0, 1, 2, 2, 3, 3, 4, 5, 5};
        for (size_t i = 0; i < expected.size(); i++) {
            auto target = c.cursorForPreeditPosition(i, mode);
            FCITX_ASSERT(target.has_value());
            FCITX_ASSERT(*target == expected[i])
                << "Preedit position " << i << " target " << *target
                << " expected " << expected[i];
        }
    }

    // Cursor mapping works when a fixed syllable renders shorter or longer
    // than its raw input.
    {
        const auto checkCursorMapping =
            [&](std::string_view rawInput, std::string_view fixedPinyin,
                PinyinInitial initial, PinyinFinal final,
                const std::vector<size_t> &inputToPreedit,
                const std::vector<std::pair<size_t, size_t>> &preeditToInput) {
                const auto mode = PinyinPreeditMode::RawText;
                for (size_t i = 0; i < inputToPreedit.size(); i++) {
                    c.clear();
                    c.type(rawInput);
                    c.setCursor(i);
                    c.setPinyinFixes({{0, rawInput.size(), {initial, final}}});

                    auto [preedit, cursor, highlights] =
                        c.preeditWithCursorHighlight(mode);
                    FCITX_ASSERT(preedit == fixedPinyin) << preedit;
                    FCITX_ASSERT(cursor == inputToPreedit[i])
                        << "Input cursor " << i << " preedit cursor " << cursor
                        << " expected " << inputToPreedit[i];
                    FCITX_ASSERT(highlights.size() == 1);
                }

                c.clear();
                c.type(rawInput);
                c.setPinyinFixes({{0, rawInput.size(), {initial, final}}});
                auto [preedit, cursor, highlights] =
                    c.preeditWithCursorHighlight(mode);
                FCITX_ASSERT(preedit == fixedPinyin) << preedit;
                FCITX_ASSERT(preeditToInput.front().first == 0);
                FCITX_ASSERT(preeditToInput.back().first == preedit.size());
                for (const auto &[preeditPos, inputCursor] : preeditToInput) {
                    auto target = c.cursorForPreeditPosition(preeditPos, mode);
                    FCITX_ASSERT(target.has_value());
                    FCITX_ASSERT(*target == inputCursor)
                        << "Preedit cursor " << preeditPos << " input cursor "
                        << *target << " expected " << inputCursor;
                }
                (void)cursor;
                (void)highlights;
            };

        // "shuang" fixed to "san".
        checkCursorMapping("shuang", "san", PinyinInitial::S, PinyinFinal::AN,
                           {0, 1, 2, 2, 2, 2, 3},
                           {{0, 0}, {1, 1}, {2, 2}, {3, 6}});
        // "san" fixed to "shuang".
        checkCursorMapping(
            "san", "shuang", PinyinInitial::SH, PinyinFinal::UANG, {0, 1, 2, 6},
            {{0, 0}, {1, 1}, {2, 2}, {3, 2}, {4, 2}, {5, 2}, {6, 3}});
        // "shuang" fixed to "nü" renders 2 code points in 3 bytes. A raw
        // cursor inside the fix stays before the rendered end, so that only
        // the rendered end maps back to the fix end. Byte 2 is in the middle
        // of "ü", so only character boundaries are checked.
        checkCursorMapping("shuang", "nü", PinyinInitial::N, PinyinFinal::V,
                           {0, 1, 1, 1, 1, 1, 3}, {{0, 0}, {1, 1}, {3, 6}});
    }

    // The end of a fixed syllable maps to the fix end. Positions inside the
    // rendered text keep the linear mapping and stay before the fix end, so
    // that entering the fix still cancels it.
    {
        c.clear();
        c.type("sui");
        c.setPinyinFixes({{0, 3, {PinyinInitial::SH, PinyinFinal::UI}}});
        const auto mode = PinyinPreeditMode::RawText;
        auto [preedit, cursor, highlights] = c.preeditWithCursorHighlight(mode);
        FCITX_ASSERT(preedit == "shui") << preedit;
        (void)cursor;
        (void)highlights;

        FCITX_ASSERT(c.cursorForPreeditPosition(0, mode).value() == 0);
        FCITX_ASSERT(c.cursorForPreeditPosition(1, mode).value() == 1);
        FCITX_ASSERT(c.cursorForPreeditPosition(3, mode).value() == 2);
        FCITX_ASSERT(c.cursorForPreeditPosition(4, mode).value() == 3);
        FCITX_ASSERT(c.cursorForPreeditPosition(5, mode).value() == 3);

        // The same rule applies when the fix is not at the end of the input.
        // In particular, only the rendered end maps to the raw fix end.
        c.clear();
        c.type("suihao");
        c.setPinyinFixes({{0, 3, {PinyinInitial::SH, PinyinFinal::UI}}});
        std::tie(preedit, cursor, highlights) =
            c.preeditWithCursorHighlight(mode);
        FCITX_ASSERT(preedit == "shui hao") << preedit;
        (void)cursor;
        (void)highlights;

        const std::array<size_t, 9> expected = {0, 1, 2, 2, 3, 3, 4, 5, 6};
        for (size_t i = 0; i < expected.size(); i++) {
            auto target = c.cursorForPreeditPosition(i, mode);
            FCITX_ASSERT(target.has_value());
            FCITX_ASSERT(*target == expected[i])
                << "Preedit position " << i << " target " << *target
                << " expected " << expected[i];
        }
    }

    // Positions inside the selected sentence have no input cursor.
    {
        c.clear();
        c.type("nuhaoxian");
        c.select(0);
        const auto mode = PinyinPreeditMode::RawText;
        FCITX_ASSERT(!c.selectedSentence().empty());
        FCITX_ASSERT(!c.cursorForPreeditPosition(0, mode).has_value());
        auto target =
            c.cursorForPreeditPosition(c.selectedSentence().size(), mode);
        FCITX_ASSERT(target.has_value());
        FCITX_ASSERT(*target == c.selectedLength());
        c.cancel();
    }
    // Editing the input cancels the affected fixes.
    {
        c.clear();
        c.type("nuhaoxian");
        c.setPinyinFixes({{0, 2, {PinyinInitial::N, PinyinFinal::I}}});
        // Typing at the end does not affect the fix.
        c.type("hao");
        FCITX_ASSERT(c.pinyinFixes().size() == 1);
        // Erasing after the fix does not affect it.
        c.erase(11, 12);
        FCITX_ASSERT(c.pinyinFixes().size() == 1);
        // Erasing from the fix boundary does not affect it.
        c.erase(2, 4);
        FCITX_ASSERT(c.pinyinFixes().size() == 1);
        // Erasing from the fix itself cancels it.
        c.erase(1, 3);
        FCITX_ASSERT(c.pinyinFixes().empty());
    }

    // Moving the cursor cancels all fixes from the cursor to the end.
    {
        const auto setup = [&]() {
            c.clear();
            c.type("nuhaoxian");
            c.setPinyinFixes({{0, 2, {PinyinInitial::N, PinyinFinal::I}},
                              {5, 7, {PinyinInitial::X, PinyinFinal::I}}});
            FCITX_ASSERT(c.pinyinFixes().size() == 2);
        };

        // Cursor at the end of the first fix keeps that fix but cancels the
        // later one.
        setup();
        c.setCursor(2);
        FCITX_ASSERT(c.pinyinFixes().size() == 1);
        FCITX_ASSERT(c.pinyinFixes().front().end_ == 2);

        // Cursor between the fixes keeps the earlier one only.
        setup();
        c.setCursor(3);
        FCITX_ASSERT(c.pinyinFixes().size() == 1);
        FCITX_ASSERT(c.pinyinFixes().front().end_ == 2);

        // Cursor inside a fix cancels it and everything after it.
        setup();
        c.setCursor(6);
        FCITX_ASSERT(c.pinyinFixes().size() == 1);
        FCITX_ASSERT(c.pinyinFixes().front().end_ == 2);

        // Cursor at the very beginning cancels every fix.
        setup();
        c.setCursor(0);
        FCITX_ASSERT(c.pinyinFixes().empty());

        // Cursor at the end keeps every fix.
        setup();
        c.setCursor(9);
        FCITX_ASSERT(c.pinyinFixes().size() == 2);
    }

    // Fixes work the same in shuangpin.
    {
        c.clear();
        ime.setShuangpinProfile(std::make_shared<ShuangpinProfile>(
            ShuangpinBuiltinProfile::Ziranma));
        c.setUseShuangpin(true);
        c.type("nu");
        checkCandidateSet(c);
        FCITX_ASSERT(c.candidateSet().count("怒") > 0);
        FCITX_ASSERT(c.candidateSet().count("你") == 0);

        const auto mode = PinyinPreeditMode::RawText;
        auto [preedit, cursor] = c.preeditWithCursor(mode);
        (void)cursor;
        FCITX_ASSERT(c.cursorForPreeditPosition(0, mode).value() == 0);
        FCITX_ASSERT(c.cursorForPreeditPosition(1, mode).value() == 1);
        FCITX_ASSERT(c.cursorForPreeditPosition(preedit.size(), mode).value() ==
                     c.size());

        // Force "nu" to be read as "ni".
        c.setPinyinFixes({{0, 2, {PinyinInitial::N, PinyinFinal::I}}});
        checkCandidateSet(c);
        FCITX_ASSERT(c.candidateSet().count("你") > 0);
        FCITX_ASSERT(c.candidateSet().count("怒") == 0);

        c.setPinyinFixes({});
        c.setUseShuangpin(false);
        c.clear();
        ime.setShuangpinProfile(nullptr);
    }

    return 0;
}
