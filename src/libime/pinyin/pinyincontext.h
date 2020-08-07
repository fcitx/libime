/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_PINYIN_PINYINCONTEXT_H_
#define _FCITX_LIBIME_PINYIN_PINYINCONTEXT_H_

#include "libimepinyin_export.h"
#include <fcitx-utils/macros.h>
#include <libime/core/inputbuffer.h>
#include <libime/core/lattice.h>
#include <memory>
#include <vector>

namespace libime {
class PinyinIME;
class PinyinContextPrivate;
class LatticeNode;
enum class PinyinPreeditMode;

class LIBIMEPINYIN_EXPORT PinyinContext : public InputBuffer {
public:
    PinyinContext(PinyinIME *ime);
    virtual ~PinyinContext();

    void setUseShuangpin(bool sp);
    bool useShuangpin() const;

    void erase(size_t from, size_t to) override;
    void setCursor(size_t pos) override;

    int maxSentenceLength() const;
    void setMaxSentenceLength(int length);

    const std::vector<SentenceResult> &candidates() const;
    void select(size_t idx);
    void cancel();
    bool cancelTill(size_t pos);

    /// Whether the input is fully selected.
    bool selected() const;

    /// The sentence for this context, can be used as preedit.
    std::string sentence() const {
        const auto &c = candidates();
        if (!c.empty()) {
            return selectedSentence() + c[0].toString();
        }
        return selectedSentence();
    }

    std::string preedit(PinyinPreeditMode mode) const;

    /// Mixed preedit (selected hanzi + pinyin).
    std::pair<std::string, size_t>
    preeditWithCursor(PinyinPreeditMode mode) const;

    std::string preedit() const;

    /// Mixed preedit (selected hanzi + pinyin).
    std::pair<std::string, size_t> preeditWithCursor() const;

    /// Selected hanzi.
    std::string selectedSentence() const;

    /// Selected pinyin length.
    size_t selectedLength() const;

    /// Selected hanzi segments.
    std::vector<std::string> selectedWords() const;

    /// Get the full pinyin string of the selected part.
    std::string selectedFullPinyin() const;

    /// Get the full pinyin string of certain candidate.
    std::string candidateFullPinyin(size_t i) const;

    /// Add the selected part to history if selected() == true.
    void learn();

    /// Return the position of last pinyin. E.g. 你h|ao, return the offset
    /// before h.
    int pinyinBeforeCursor() const;

    /// Return the position of last pinyin. E.g. 你h|ao, return the offset after
    /// h.
    int pinyinAfterCursor() const;

    PinyinIME *ime() const;

    /// Opaque language model state.
    State state() const;

protected:
    bool typeImpl(const char *s, size_t length) override;

private:
    void update();
    bool learnWord();
    std::unique_ptr<PinyinContextPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(PinyinContext);
};
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINCONTEXT_H_
