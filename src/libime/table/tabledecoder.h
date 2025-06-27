/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_LIBIME_TABLE_TABLEDECODER_H_
#define _FCITX_LIBIME_TABLE_TABLEDECODER_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <libime/core/decoder.h>
#include <libime/core/languagemodel.h>
#include <libime/core/lattice.h>
#include <libime/core/segmentgraph.h>
#include <libime/table/libimetable_export.h>
#include <libime/table/tablebaseddictionary.h>

namespace libime {

class TableLatticeNodePrivate;

class LIBIMETABLE_EXPORT TableLatticeNode : public LatticeNode {
public:
    TableLatticeNode(std::string_view word, WordIndex idx,
                     SegmentGraphPath path, const State &state, float cost,
                     std::unique_ptr<TableLatticeNodePrivate> data);
    virtual ~TableLatticeNode();

    uint32_t index() const;
    PhraseFlag flag() const;
    const std::string &code() const;
    size_t codeLength() const;

private:
    std::unique_ptr<TableLatticeNodePrivate> d_ptr;
};

class LIBIMETABLE_EXPORT TableDecoder : public Decoder {
public:
    TableDecoder(const TableBasedDictionary *dict,
                 const LanguageModelBase *model)
        : Decoder(dict, model) {}

protected:
    LatticeNode *createLatticeNodeImpl(const SegmentGraphBase &graph,
                                       const LanguageModelBase *model,
                                       std::string_view word, WordIndex idx,
                                       SegmentGraphPath path,
                                       const State &state, float cost,
                                       std::unique_ptr<LatticeNodeData> data,
                                       bool onlyPath) const override;

    // When segment graph is extremely simple, no need to sort because context
    // will sort it anyway.
    bool needSort(const SegmentGraph &graph,
                  const SegmentGraphNode *node) const override;
};

LIBIMETABLE_EXPORT SegmentGraph graphForCode(std::string_view s,
                                             const TableBasedDictionary &dict);
} // namespace libime

#endif // _FCITX_LIBIME_TABLE_TABLEDECODER_H_
