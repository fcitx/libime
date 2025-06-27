/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_USERLANGUAGEMODEL_H_
#define _FCITX_LIBIME_CORE_USERLANGUAGEMODEL_H_

#include <istream>
#include <memory>
#include <ostream>
#include <string_view>
#include <fcitx-utils/macros.h>
#include <libime/core/languagemodel.h>
#include <libime/core/libimecore_export.h>

namespace libime {

class UserLanguageModelPrivate;
class HistoryBigram;

class LIBIMECORE_EXPORT UserLanguageModel : public LanguageModel {
public:
    explicit UserLanguageModel(const char *sysfile);

    UserLanguageModel(
        std::shared_ptr<const StaticLanguageModelFile> file = nullptr);
    virtual ~UserLanguageModel();

    HistoryBigram &history();
    const HistoryBigram &history() const;
    void load(std::istream &in);
    void save(std::ostream &out);

    void setHistoryWeight(float w);
    float historyWeight() const;

    void setUseOnlyUnigram(bool useOnlyUnigram);
    bool useOnlyUnigram() const;

    const State &beginState() const override;
    const State &nullState() const override;
    float score(const State &state, const WordNode &word,
                State &out) const override;
    bool isUnknown(WordIndex idx, std::string_view view) const override;

private:
    std::unique_ptr<UserLanguageModelPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(UserLanguageModel);
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_USERLANGUAGEMODEL_H_
