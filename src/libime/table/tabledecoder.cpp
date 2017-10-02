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
}

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

TableLatticeNode::TableLatticeNode(
    boost::string_view word, WordIndex idx, SegmentGraphPath path,
    const State &state, float cost,
    std::unique_ptr<TableLatticeNodePrivate> data)
    : LatticeNode(word, idx, std::move(path), state, cost),
      d_ptr(std::move(data)) {}

TableLatticeNode::~TableLatticeNode() = default;

LatticeNode *TableDecoder::createLatticeNodeImpl(
    const SegmentGraphBase &, const LanguageModelBase *,
    boost::string_view word, WordIndex idx, SegmentGraphPath path,
    const State &state, float cost, std::unique_ptr<LatticeNodeData> data,
    bool) const {
    std::unique_ptr<TableLatticeNodePrivate> tableData(
        static_cast<TableLatticeNodePrivate *>(data.release()));
    return new TableLatticeNode(word, idx, std::move(path), state, cost,
                                std::move(tableData));
}

LIBIMETABLE_EXPORT SegmentGraph graphForCode(boost::string_view s,
                                             const TableBasedDictionary &dict) {
    SegmentGraph graph(s.to_string());
    if (s.empty()) {
        return graph;
    }
    graph.addNext(0, graph.size());
    auto codeLength = fcitx::utf8::length(graph.data());
    // Rule.
    if (dict.hasRule() && dict.tableOptions().autoRuleSet().size()) {
        const auto &ruleSet = dict.tableOptions().autoRuleSet();
        for (const auto &ruleName : ruleSet) {
            auto rule = dict.findRule(ruleName);
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
}
