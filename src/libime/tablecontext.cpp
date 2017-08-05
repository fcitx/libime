/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#include "tablecontext.h"
#include "decoder.h"
#include "segmentgraph.h"
#include "tablebaseddictionary.h"
#include "tableoptions.h"
#include "userlanguagemodel.h"
#include <fcitx-utils/log.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/utf8.h>

namespace libime {

class TableContextPrivate {
public:
    TableContextPrivate(TableBasedDictionary &dict, UserLanguageModel &model)
        : dict_(dict), model_(model), decoder_(&dict, &model) {}

    TableBasedDictionary &dict_;
    UserLanguageModel &model_;
    Decoder decoder_;
    Lattice lattice_;
    SegmentGraph graph_;
};

TableContext::TableContext(TableBasedDictionary &dict, UserLanguageModel &model)
    : InputBuffer(true),
      d_ptr(std::make_unique<TableContextPrivate>(dict, model)) {}

TableContext::~TableContext() {}

const TableBasedDictionary &TableContext::dict() const {
    FCITX_D();
    return d->dict_;
}

bool TableContext::cancelTill(size_t pos) { return false; }

bool TableContext::isValidInput(char c) const {
    FCITX_D();
    if (d->dict_.isInputCode(c)) {
        return true;
    }

    if (d->dict_.tableOptions().matchingKey() == c) {
        return true;
    }

    if (d->dict_.tableOptions().pinyinKey() == c) {
        return true;
    }
    if (d->dict_.tableOptions().pinyinKey()) {
        const boost::string_view validPinyin("abcdefghijklmnopqrstuvwxyz");
        if (validPinyin.find(c) != boost::string_view::npos) {
            return true;
        }
    }

    return false;
}

void TableContext::typeImpl(const char *s, size_t length) {
    cancelTill(cursor());
    boost::string_view view(s, length);
    auto utf8len = fcitx::utf8::lengthValidated(view.begin(), view.end());
    if (utf8len != fcitx::utf8::INVALID_LENGTH && utf8len > 0) {
        for (auto iter = view.begin(), next = fcitx::utf8::nextChar(iter);
             next <= view.end();
             iter = next, next = fcitx::utf8::nextChar(next)) {
            typeOneChar(iter, next - iter);
            update();
        }
    }
}

void TableContext::setCursor(size_t pos) {
    auto cancelled = cancelTill(pos);
    InputBuffer::setCursor(pos);
    if (cancelled) {
        update();
    }
}

void TableContext::erase(size_t from, size_t to) {
    if (from == to) {
        return;
    }

    // check if erase everything
    if (from == 0 && to >= size()) {
        FCITX_D();
        // FIXME
    } else {
        cancelTill(from);
    }
    InputBuffer::erase(from, to);

    if (size()) {
        update();
    }
}

void TableContext::update() {}

boost::string_view lastSegment(const SegmentGraph &graph) {
    if (!graph.size()) {
        return {};
    } else {
        assert(graph.end().prevSize());
        return graph.segment(graph.end().prevs().front(), graph.end());
    }
}

void TableContext::typeOneChar(const char *s, size_t length) {
    FCITX_D();
    InputBuffer::typeImpl(s, length);
    auto &option = d->dict_.tableOptions();
    if (option.pinyinKey() &&
        fcitx::stringutils::startsWith(userInput(), option.pinyinKey())) {
        // TODO: do pinyin stuff.
        return;
    }

    d->graph_.appendToLast({s, length});
    auto lastSeg = lastSegment(d->graph_);
    d->decoder_.decode(d->lattice_, d->graph_, 1, d->model_.nullState());
    // TODO handle exact match
    std::vector<std::pair<std::string, std::string>> candidates;
    d->dict_.matchWords(lastSeg, TableMatchMode::Prefix,
                        [&candidates](boost::string_view code,
                                      boost::string_view word, float cost) {
                            candidates.emplace_back(code.to_string(),
                                                    word.to_string());
                            return true;
                        });
    if (candidates.size()) {
        for (auto &candidate : candidates) {
            FCITX_LOG(Info) << candidate.first << " " << candidate.second;
        }
    }
}
}
