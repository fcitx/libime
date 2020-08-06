/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "tabledecoder_p.h"
#include "tableoptions.h"
#include "tablerule.h"
#include <boost/range/adaptor/filtered.hpp>
#include <cmath>
#include <fcitx-utils/utf8.h>

namespace libime {

namespace {

bool isNotPlaceHolder(const TableRuleEntry &entry) {
    return !entry.isPlaceHolder();
}

bool checkRuleCanBeUsedAsAutoRule(const TableRule &rule) {
    if (rule.flag() != TableRuleFlag::LengthEqual) {
        return false;
    }

    auto range = rule.entries() | boost::adaptors::filtered(isNotPlaceHolder);
    auto iter = std::begin(range);
    auto end = std::end(range);
    int currentChar = 1;
    while (iter != end) {
        int currentIndex = 1;
        while (iter != end) {
            if (iter->character() == currentChar) {
                if (iter->flag() == TableRuleEntryFlag::FromFront &&
                    iter->encodingIndex() == currentIndex) {
                    currentIndex++;
                } else {
                    // reset to invalid.
                    currentIndex = 1;
                    break;
                }
            } else {
                break;
            }
            ++iter;
        }

        if (currentIndex == 1) {
            return false;
        }
        currentChar++;
    }
    return currentChar == rule.phraseLength() + 1;
}
} // namespace

uint32_t TableLatticeNode::index() const {
    return d_ptr ? d_ptr->index_ : 0xFFFFFFFFu;
}

PhraseFlag TableLatticeNode::flag() const {
    return d_ptr ? d_ptr->flag_ : PhraseFlag::None;
}

const std::string &TableLatticeNode::code() const {
    static const std::string empty;
    if (!d_ptr) {
        return empty;
    }
    return d_ptr->code_;
}

size_t TableLatticeNode::codeLength() const {
    if (!d_ptr) {
        return 0;
    }
    return d_ptr->codeLength_;
}

TableLatticeNode::TableLatticeNode(
    std::string_view word, WordIndex idx, SegmentGraphPath path,
    const State &state, float cost,
    std::unique_ptr<TableLatticeNodePrivate> data)
    : LatticeNode(word, idx, std::move(path), state, cost),
      d_ptr(std::move(data)) {}

TableLatticeNode::~TableLatticeNode() = default;

LatticeNode *TableDecoder::createLatticeNodeImpl(
    const SegmentGraphBase &, const LanguageModelBase *, std::string_view word,
    WordIndex idx, SegmentGraphPath path, const State &state, float cost,
    std::unique_ptr<LatticeNodeData> data, bool) const {
    std::unique_ptr<TableLatticeNodePrivate> tableData(
        static_cast<TableLatticeNodePrivate *>(data.release()));
    return new TableLatticeNode(word, idx, std::move(path), state, cost,
                                std::move(tableData));
}

bool TableDecoder::needSort(const SegmentGraph &graph,
                            const SegmentGraphNode *) const {
    return graph.start().nextSize() != 1;
}

LIBIMETABLE_EXPORT SegmentGraph graphForCode(std::string_view s,
                                             const TableBasedDictionary &dict) {
    SegmentGraph graph{std::string(s)};
    if (s.empty()) {
        return graph;
    }
    graph.addNext(0, graph.size());
    auto codeLength = fcitx::utf8::length(graph.data());
    // Rule.
    if (dict.hasRule() && !dict.tableOptions().autoRuleSet().empty()) {
        const auto &ruleSet = dict.tableOptions().autoRuleSet();
        for (const auto &ruleName : ruleSet) {
            const auto *rule = dict.findRule(ruleName);
            if (!rule || codeLength != rule->codeLength() ||
                !checkRuleCanBeUsedAsAutoRule(*rule)) {
                continue;
            }

            std::vector<int> charSizes(rule->phraseLength());
            for (const auto &entry :
                 rule->entries() |
                     boost::adaptors::filtered(isNotPlaceHolder)) {
                auto &charSize = charSizes[entry.character() - 1];
                charSize =
                    std::max(charSize, static_cast<int>(entry.encodingIndex()));
            }

            int lastIndex = 0;
            for (auto charSize : charSizes) {
                graph.addNext(fcitx::utf8::ncharByteLength(graph.data().begin(),
                                                           lastIndex),
                              fcitx::utf8::ncharByteLength(
                                  graph.data().begin(), lastIndex + charSize));
                lastIndex += charSize;
            }
        }
    }

    return graph;
}
} // namespace libime
