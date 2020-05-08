/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_CORE_DECODER_H_
#define _FCITX_LIBIME_CORE_DECODER_H_

#include "libimecore_export.h"
#include <cstdint>
#include <fcitx-utils/macros.h>
#include <libime/core/dictionary.h>
#include <libime/core/lattice.h>
#include <libime/core/segmentgraph.h>
#include <memory>
#include <vector>

namespace libime {

class DecoderPrivate;
class Dictionary;
class LanguageModel;

class LIBIMECORE_EXPORT Decoder {
    friend class DecoderPrivate;

public:
    Decoder(const Dictionary *dict, const LanguageModelBase *model);
    virtual ~Decoder();

    constexpr static const size_t beamSizeDefault = 20;
    constexpr static const size_t frameSizeDefault = 200;

    const Dictionary *dict() const;
    const LanguageModelBase *model() const;

    bool decode(Lattice &lattice, const SegmentGraph &graph, size_t nbest,
                const State &state,
                float max = std::numeric_limits<float>::max(),
                float min = -std::numeric_limits<float>::max(),
                size_t beamSize = beamSizeDefault,
                size_t frameSize = frameSizeDefault,
                void *helper = nullptr) const;

protected:
    inline LatticeNode *
    createLatticeNode(const SegmentGraph &graph, const LanguageModelBase *model,
                      std::string_view word, WordIndex idx,
                      SegmentGraphPath path, const State &state, float cost = 0,
                      std::unique_ptr<LatticeNodeData> data = nullptr,
                      bool onlyPath = false) const {
        return createLatticeNodeImpl(graph, model, word, idx, std::move(path),
                                     state, cost, std::move(data), onlyPath);
    }
    virtual LatticeNode *createLatticeNodeImpl(
        const SegmentGraphBase &graph, const LanguageModelBase *model,
        std::string_view word, WordIndex idx, SegmentGraphPath path,
        const State &state, float cost, std::unique_ptr<LatticeNodeData> data,
        bool onlyPath) const;

    virtual bool needSort(const SegmentGraph &,
                          const SegmentGraphNode *) const {
        return true;
    }

private:
    std::unique_ptr<DecoderPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Decoder);
};
} // namespace libime

#endif // _FCITX_LIBIME_CORE_DECODER_H_
