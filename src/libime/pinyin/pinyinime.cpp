/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "pinyinime.h"
#include "libime/core/userlanguagemodel.h"
#include "pinyindecoder.h"

namespace libime {

class PinyinIMEPrivate : fcitx::QPtrHolder<PinyinIME> {
public:
    PinyinIMEPrivate(PinyinIME *q, std::unique_ptr<PinyinDictionary> dict,
                     std::unique_ptr<UserLanguageModel> model)
        : fcitx::QPtrHolder<PinyinIME>(q), dict_(std::move(dict)),
          model_(std::move(model)),
          decoder_(std::make_unique<PinyinDecoder>(dict_.get(), model_.get())) {
    }

    FCITX_DEFINE_SIGNAL_PRIVATE(PinyinIME, optionChanged);

    PinyinFuzzyFlags flags_;
    std::unique_ptr<PinyinDictionary> dict_;
    std::unique_ptr<UserLanguageModel> model_;
    std::unique_ptr<PinyinDecoder> decoder_;
    std::shared_ptr<const ShuangpinProfile> spProfile_;
    size_t nbest_ = 1;
    size_t beamSize_ = Decoder::beamSizeDefault;
    size_t frameSize_ = Decoder::frameSizeDefault;
    size_t partialLongWordLimit_ = 0;
    float maxDistance_ = std::numeric_limits<float>::max();
    float minPath_ = -std::numeric_limits<float>::max();
    PinyinPreeditMode preeditMode_ = PinyinPreeditMode::RawText;
};

PinyinIME::PinyinIME(std::unique_ptr<PinyinDictionary> dict,
                     std::unique_ptr<UserLanguageModel> model)
    : d_ptr(std::make_unique<PinyinIMEPrivate>(this, std::move(dict),
                                               std::move(model))) {}

PinyinIME::~PinyinIME() {}

PinyinFuzzyFlags PinyinIME::fuzzyFlags() const {
    FCITX_D();
    return d->flags_;
}

void PinyinIME::setFuzzyFlags(PinyinFuzzyFlags flags) {
    FCITX_D();
    d->flags_ = flags;
    emit<PinyinIME::optionChanged>();
}

PinyinDictionary *PinyinIME::dict() {
    FCITX_D();
    return d->dict_.get();
}

const PinyinDictionary *PinyinIME::dict() const {
    FCITX_D();
    return d->dict_.get();
}

const PinyinDecoder *PinyinIME::decoder() const {
    FCITX_D();
    return d->decoder_.get();
}

UserLanguageModel *PinyinIME::model() {
    FCITX_D();
    return d->model_.get();
}

const UserLanguageModel *PinyinIME::model() const {
    FCITX_D();
    return d->model_.get();
}

size_t PinyinIME::nbest() const {
    FCITX_D();
    return d->nbest_;
}

void PinyinIME::setNBest(size_t n) {
    FCITX_D();
    if (d->nbest_ != n) {
        d->nbest_ = n;
        emit<PinyinIME::optionChanged>();
    }
}

size_t PinyinIME::beamSize() const {
    FCITX_D();
    return d->beamSize_;
}

void PinyinIME::setBeamSize(size_t n) {
    FCITX_D();
    if (d->beamSize_ != n) {
        d->beamSize_ = n;
        emit<PinyinIME::optionChanged>();
    }
}

size_t PinyinIME::frameSize() const {
    FCITX_D();
    return d->frameSize_;
}

void PinyinIME::setFrameSize(size_t n) {
    FCITX_D();
    if (d->frameSize_ != n) {
        d->frameSize_ = n;
        emit<PinyinIME::optionChanged>();
    }
}

size_t PinyinIME::partialLongWordLimit() const {
    FCITX_D();
    return d->partialLongWordLimit_;
}

void PinyinIME::setPartialLongWordLimit(size_t n) {
    FCITX_D();
    if (d->partialLongWordLimit_ != n) {
        d->partialLongWordLimit_ = n;
        emit<PinyinIME::optionChanged>();
    }
}

void PinyinIME::setPreeditMode(PinyinPreeditMode mode) {
    FCITX_D();
    if (d->preeditMode_ != mode) {
        d->preeditMode_ = mode;
        emit<PinyinIME::optionChanged>();
    }
}

PinyinPreeditMode PinyinIME::preeditMode() const {
    FCITX_D();
    return d->preeditMode_;
}

void PinyinIME::setScoreFilter(float maxDistance, float minPath) {
    FCITX_D();
    if (d->maxDistance_ != maxDistance || d->minPath_ != minPath) {
        d->maxDistance_ = maxDistance;
        d->minPath_ = minPath;
        emit<PinyinIME::optionChanged>();
    }
}

float PinyinIME::maxDistance() const {
    FCITX_D();
    return d->maxDistance_;
}

float PinyinIME::minPath() const {
    FCITX_D();
    return d->minPath_;
}

void PinyinIME::setShuangpinProfile(
    std::shared_ptr<const ShuangpinProfile> profile) {
    FCITX_D();
    if (d->spProfile_ != profile) {
        d->spProfile_ = profile;
        emit<PinyinIME::optionChanged>();
    }
}

std::shared_ptr<const ShuangpinProfile> PinyinIME::shuangpinProfile() const {
    FCITX_D();
    return d->spProfile_;
}
} // namespace libime
