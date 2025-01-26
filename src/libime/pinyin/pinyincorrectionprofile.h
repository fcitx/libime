/*
 * SPDX-FileCopyrightText: 2024-2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_PINYIN_PINYINCORRECTIONPROFILE_H_
#define _FCITX_LIBIME_PINYIN_PINYINCORRECTIONPROFILE_H_

#include <fcitx-utils/macros.h>
#include <libime/pinyin/libimepinyin_export.h>
#include <libime/pinyin/pinyindata.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace libime {

/**
 * Built-in pinyin profile mapping
 *
 * @since 1.1.7
 */
enum class BuiltinPinyinCorrectionProfile {
    /**
     * Pinyin correction based on qwerty keyboard
     */
    Qwerty,
};

class PinyinCorrectionProfilePrivate;

/**
 * Class that holds updated Pinyin correction mapping based on correction
 * mapping.
 * @since 1.1.7
 */
class LIBIMEPINYIN_EXPORT PinyinCorrectionProfile {
public:
    /**
     * Construct the profile based on builtin layout.
     *
     * @param profile built-in profile
     */
    explicit PinyinCorrectionProfile(BuiltinPinyinCorrectionProfile profile);

    /**
     * Construct the profile based on customized mapping.
     *
     * E.g. w may be corrected to q,e, the mapping will contain {'w': ['q',
     * 'e']}.
     *
     * @param mapping pinyin character and the corresponding possible wrong key.
     */
    explicit PinyinCorrectionProfile(
        const std::unordered_map<char, std::vector<char>> &mapping);

    virtual ~PinyinCorrectionProfile();

    /**
     * Return the updated pinyin map
     *
     * New entries will be marked with PinyinFuzzyFlag::Correction
     *
     * @see getPinyinMapV2
     */
    const PinyinMap &pinyinMap() const;
    /**
     * Return the correction mapping.
     *
     * E.g. w may be corrected to q,e, the mapping will contain {'w': ['q',
     * 'e']}.
     *
     * @see getPinyinMapV2
     */
    const std::unordered_map<char, std::vector<char>> &correctionMap() const;

private:
    FCITX_DECLARE_PRIVATE(PinyinCorrectionProfile);
    std::unique_ptr<PinyinCorrectionProfilePrivate> d_ptr;
};

} // namespace libime

#endif
