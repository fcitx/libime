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

class LIBIMEPINYIN_EXPORT PinyinContext : public InputBuffer {
public:
    PinyinContext(PinyinIME *ime);
    virtual ~PinyinContext();

    void setUseShuangpin(bool sp);
    bool useShuangpin() const;

    void erase(size_t from, size_t to) override;
    void setCursor(size_t pos) override;

    const std::vector<SentenceResult> &candidates() const;
    void select(size_t idx);
    void cancel();
    bool cancelTill(size_t pos);

    bool selected() const;
    std::string sentence() const {
        auto &c = candidates();
        if (c.size()) {
            return selectedSentence() + c[0].toString();
        } else {
            return selectedSentence();
        }
    }

    std::string preedit() const;
    std::pair<std::string, size_t> preeditWithCursor() const;
    std::string selectedSentence() const;
    size_t selectedLength() const;

    std::vector<std::string> selectedWords() const;

    std::string selectedFullPinyin() const;
    std::string candidateFullPinyin(size_t i) const;

    void learn();

    int pinyinBeforeCursor() const;
    int pinyinAfterCursor() const;

    PinyinIME *ime() const;

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
