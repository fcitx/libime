/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_PINYIN_PINYINIME_H_
#define _FCITX_LIBIME_PINYIN_PINYINIME_H_

#include <cstddef>
#include <limits>
#include <memory>
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/macros.h>
#include <libime/pinyin/libimepinyin_export.h>
#include <libime/pinyin/pinyincorrectionprofile.h>
#include <libime/pinyin/pinyinencoder.h>

namespace libime {

class PinyinIMEPrivate;
class PinyinDecoder;
class PinyinDictionary;
class UserLanguageModel;

enum class PinyinPreeditMode { RawText, Pinyin };

/// \brief Provides shared data for PinyinContext.
class LIBIMEPINYIN_EXPORT PinyinIME : public fcitx::ConnectableObject {
public:
    PinyinIME(std::unique_ptr<PinyinDictionary> dict,
              std::unique_ptr<UserLanguageModel> model);
    virtual ~PinyinIME();

    PinyinFuzzyFlags fuzzyFlags() const;
    void setFuzzyFlags(PinyinFuzzyFlags flags);
    size_t nbest() const;
    void setNBest(size_t n);
    size_t beamSize() const;
    void setBeamSize(size_t n);
    size_t frameSize() const;
    void setFrameSize(size_t n);
    size_t partialLongWordLimit() const;
    void setPartialLongWordLimit(size_t n);
    void setScoreFilter(float maxDistance = std::numeric_limits<float>::max(),
                        float minPath = -std::numeric_limits<float>::max());
    void setShuangpinProfile(std::shared_ptr<const ShuangpinProfile> profile);
    std::shared_ptr<const ShuangpinProfile> shuangpinProfile() const;
    void setPreeditMode(PinyinPreeditMode mode);
    PinyinPreeditMode preeditMode() const;

    void setCorrectionProfile(
        std::shared_ptr<const PinyinCorrectionProfile> profile);
    std::shared_ptr<const PinyinCorrectionProfile> correctionProfile() const;

    float maxDistance() const;
    float minPath() const;

    PinyinDictionary *dict();
    const PinyinDictionary *dict() const;
    const PinyinDecoder *decoder() const;
    UserLanguageModel *model();
    const UserLanguageModel *model() const;

    FCITX_DECLARE_SIGNAL(PinyinIME, optionChanged, void());

private:
    std::unique_ptr<PinyinIMEPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(PinyinIME);
};
} // namespace libime

#endif // _FCITX_LIBIME_PINYIN_PINYINIME_H_
