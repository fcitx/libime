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
                      boost::string_view word, WordIndex idx,
                      SegmentGraphPath path, const State &state, float cost = 0,
                      std::unique_ptr<LatticeNodeData> data = nullptr,
                      bool onlyPath = false) const {
        return createLatticeNodeImpl(graph, model, word, idx, std::move(path),
                                     state, cost, std::move(data), onlyPath);
    }
    virtual LatticeNode *createLatticeNodeImpl(
        const SegmentGraphBase &graph, const LanguageModelBase *model,
        boost::string_view word, WordIndex idx, SegmentGraphPath path,
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
}

#endif // _FCITX_LIBIME_CORE_DECODER_H_
