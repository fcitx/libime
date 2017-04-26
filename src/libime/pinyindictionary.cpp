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

#include "pinyindictionary.h"
#include "datrie.h"
#include "pinyinencoder.h"
#include "utils.h"
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/utility/string_view.hpp>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <queue>
#include <type_traits>

namespace libime {

using PinyinTrie = DATrie<float>;

static const float fuzzyCost = std::log10(0.5f);
static const float invalidPinyinCost = -100.0f;

class PinyinDictionaryPrivate {
public:
    boost::ptr_vector<PinyinTrie> tries_;
    PinyinFuzzyFlags flags_;
};

size_t numOfFuzzy(const SegmentGraph &seg, const SegmentGraphPath &path,
                  boost::string_view encodedPinyin) {
    size_t count = 0;
    size_t j = 0;
    auto finalOffset = encodedPinyin.size() / 2 + 1;
    for (size_t i = 0; i < path.size() - 1; i++) {
        auto pinyin = seg.segment(path[i]->index(), path[i + 1]->index());
        if (boost::starts_with(pinyin, "\'")) {
            continue;
        }
        auto initial = encodedPinyin[j];
        auto final = encodedPinyin[j + finalOffset];
        j++;
        auto &initialString =
            PinyinEncoder::initialToString(static_cast<PinyinInitial>(initial));
        auto &finalString =
            PinyinEncoder::finalToString(static_cast<PinyinFinal>(final));
        if (initialString.size() + finalString.size() != pinyin.size() ||
            !boost::starts_with(pinyin, initialString) ||
            !boost::ends_with(pinyin, finalString)) {
            assert(initialString + finalString != pinyin);
            count++;
        }
    }
    return count;
}

struct TrieEdge {

    TrieEdge(PinyinTrie *trie, uint64_t pos, SegmentGraphPath path,
             std::vector<std::vector<PinyinFinal>> remain)
        : trie_(trie), pos_(pos), path_(std::move(path)),
          remain_(std::move(remain)) {}

    PinyinTrie *trie_;
    uint64_t pos_;
    SegmentGraphPath path_;
    std::vector<std::vector<PinyinFinal>> remain_;
};

struct SegmentGraphNodeGreater {
    bool operator()(const SegmentGraphNode *lhs,
                    const SegmentGraphNode *rhs) const {
        return lhs->index() > rhs->index();
    }
};

const SegmentGraphNode *prevIsSeparator(const SegmentGraph &graph,
                                        const SegmentGraphNode *node) {
    auto prev = node->prev();
    if (!prev.empty()) {
        auto iter = prev.begin();
        ++iter;
        if (iter == prev.end()) {
            auto i = prev.begin();
            auto pinyin = graph.segment(i->index(), node->index());
            if (boost::starts_with(pinyin, "\'")) {
                return &(*i);
            }
        }
    }
    return nullptr;
}

void PinyinDictionary::matchPrefixImpl(
    const SegmentGraph &graph, GraphMatchCallback callback,
    const std::unordered_set<const SegmentGraphNode *> &ignore) {
    FCITX_D();

    std::unordered_map<const SegmentGraphNode *, std::list<TrieEdge>> search;
    std::priority_queue<const SegmentGraphNode *,
                        std::vector<const SegmentGraphNode *>,
                        SegmentGraphNodeGreater>
        q;

    auto &start = graph.start();
    q.push(&start);

    while (!q.empty()) {
        auto current = q.top();
        q.pop();

        for (auto &node : current->next()) {
            q.push(&node);
        }

        if (search.count(current)) {
            continue;
        }

        auto &currentNodes = search[current];
        if (current != &graph.end() &&
            !boost::starts_with(
                graph.segment(current->index(), current->index() + 1), "\'")) {
            SegmentGraphPath vec;
            if (auto prev = prevIsSeparator(graph, current)) {
                vec.push_back(prev);
            }

            vec.push_back(current);
            for (auto &trie : d->tries_) {
                currentNodes.emplace_back(
                    &trie, 0, vec, std::vector<std::vector<PinyinFinal>>());
            }
        }

        for (auto &prevNode : current->prev()) {
            auto pinyin = graph.segment(prevNode.index(), current->index());
            if (boost::starts_with(pinyin, "\'")) {
                auto newNodes = search[&prevNode];
                for (auto &edge : newNodes) {
                    edge.path_.push_back(current);
                }
                currentNodes.splice(currentNodes.end(), newNodes);
                if (current == &graph.end()) {
                    callback({&prevNode, current}, "", 0, "");
                }
                continue;
            }

            bool matched = false;
            const auto syls =
                PinyinEncoder::stringToSyllables(pinyin, d->flags_);
            auto &nodes = search[&prevNode];
            std::remove_reference_t<decltype(nodes)> newNodes;
            for (auto &node : nodes) {
                for (auto &syl : syls) {
                    char initial = static_cast<char>(syl.first);
                    auto pos = node.pos_;
                    auto result = node.trie_->traverse(&initial, 1, pos);
                    if (!PinyinTrie::isNoPath(result)) {
                        auto finals = node.remain_;
                        finals.push_back(syl.second);
                        auto v = node.path_;
                        v.push_back(current);
                        newNodes.emplace_back(node.trie_, pos, std::move(v),
                                              std::move(finals));
                    }
                }
            }

            if (!ignore.count(current)) {
                // after we match initial, we first try to match final for
                // current word.
                for (const auto &node : newNodes) {
                    char sep = PinyinEncoder::initialFinalSepartor;
                    auto pos = node.pos_;
                    auto result = node.trie_->traverse(&sep, 1, pos);
                    if (PinyinTrie::isNoPath(result)) {
                        continue;
                    }

                    std::list<PinyinTrie::position_type> finalNodes = {pos};
                    for (auto finals : node.remain_) {
                        decltype(finalNodes) nextNodes;

                        for (auto pos : finalNodes) {
                            auto updateNext = [&node, &nextNodes](char current,
                                                                  auto pos) {
                                auto result =
                                    node.trie_->traverse(&current, 1, pos);

                                if (!PinyinTrie::isNoPath(result)) {
                                    nextNodes.push_back(pos);
                                }
                            };
                            if (finals.size() > 1 ||
                                finals[0] != PinyinFinal::Invalid) {
                                for (auto final : finals) {
                                    updateNext(static_cast<char>(final), pos);
                                }
                            } else {
                                for (char test = PinyinEncoder::firstFinal;
                                     test <= PinyinEncoder::lastFinal; test++) {
                                    updateNext(test, pos);
                                }
                            }
                        }
                        finalNodes = std::move(nextNodes);
                        if (finalNodes.empty()) {
                            break;
                        }
                    }

                    for (auto finalNode : finalNodes) {
                        node.trie_->foreach (
                            [d, &callback, &node, &graph, &matched,
                             &prevNode](PinyinTrie::value_type value,
                                        size_t len, uint64_t pos) {
                                auto size = node.remain_.size();
                                std::string s;
                                node.trie_->suffix(s, len + size * 2 + 1, pos);
                                auto view = boost::string_view(s);
                                auto encodedPinyin =
                                    view.substr(0, size * 2 + 1);
                                size_t fuzzy = numOfFuzzy(graph, node.path_,
                                                          encodedPinyin);
                                callback(node.path_, view.substr(size * 2 + 1),
                                         value + fuzzy * fuzzyCost,
                                         encodedPinyin);
                                if (node.remain_.size() == 1 &&
                                    node.path_[node.path_.size() - 2] ==
                                        &prevNode) {
                                    matched = true;
                                }
                                return true;
                            },
                            finalNode);
                    }
                }

                if (!matched) {
                    SegmentGraphPath vec;
                    vec.reserve(3);
                    if (auto prevPrev = prevIsSeparator(graph, &prevNode)) {
                        vec.push_back(prevPrev);
                    }
                    vec.push_back(&prevNode);
                    vec.push_back(current);
                    callback(vec, pinyin, invalidPinyinCost, "");
                }
            }

            currentNodes.splice(currentNodes.end(), newNodes);
        }
    }
}

void PinyinDictionary::matchWords(const char *data, size_t size,
                                  PinyinMatchCallback callback) {
    if (size % 2 != 1 ||
        data[size / 2] != PinyinEncoder::initialFinalSepartor) {
        throw std::invalid_argument("invalid pinyin");
    }

    return matchWords(data, data + size / 2 + 1, size / 2, callback);
}

void PinyinDictionary::matchWords(const char *initials, const char *finals,
                                  size_t size, PinyinMatchCallback callback) {
    FCITX_D();
    for (size_t i = 0; i < size; i++) {
        if (!PinyinEncoder::isValidInitial(initials[i])) {
            throw std::invalid_argument("invalid pinyin");
        }
    }

    std::list<std::pair<PinyinTrie *, PinyinTrie::position_type>> nodes;
    for (auto &trie : d->tries_) {
        nodes.emplace_back(&trie, 0);
    }
    for (size_t i = 0, e = size * 2 + 1; i < e && nodes.size(); i++) {
        char current;
        if (i < size) {
            current = initials[i];
        } else if (i > size) {
            current = finals[i - size - 1];
        } else {
            current = PinyinEncoder::initialFinalSepartor;
        }
        decltype(nodes) extraNodes;
        auto iter = nodes.begin();
        while (iter != nodes.end()) {
            if (current != 0) {
                PinyinTrie::value_type result;
                result = iter->first->traverse(&current, 1, iter->second);

                if (PinyinTrie::isNoPath(result)) {
                    nodes.erase(iter++);
                } else {
                    iter++;
                }
            } else {
                bool changed = false;
                for (char test = PinyinEncoder::firstFinal;
                     test <= PinyinEncoder::lastFinal; test++) {
                    decltype(extraNodes)::value_type p = *iter;
                    auto result = p.first->traverse(&test, 1, p.second);
                    if (!PinyinTrie::isNoPath(result)) {
                        extraNodes.push_back(p);
                        changed = true;
                    }
                }
                if (changed) {
                    *iter = extraNodes.back();
                    extraNodes.pop_back();
                    iter++;
                } else {
                    nodes.erase(iter++);
                }
            }
        }
        nodes.splice(nodes.end(), std::move(extraNodes));
    }

    for (auto &node : nodes) {
        node.first->foreach (
            [&node, &callback, size](PinyinTrie::value_type value, size_t len,
                                     uint64_t pos) {
                std::string s;
                node.first->suffix(s, len + size * 2 + 1, pos);
                return callback(s.c_str(), s.substr(size * 2 + 1), value);
            },
            node.second);
    }
}
PinyinDictionary::PinyinDictionary()
    : d_ptr(std::make_unique<PinyinDictionaryPrivate>()) {
    addEmptyDict();
    addEmptyDict();
}

PinyinDictionary::~PinyinDictionary() {}

void PinyinDictionary::addEmptyDict() {
    FCITX_D();
    d->tries_.push_back(new PinyinTrie);
}

void PinyinDictionary::load(size_t idx, const char *filename,
                            PinyinDictFormat format) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    throw_if_io_fail(in);
    load(idx, in, format);
}

void PinyinDictionary::load(size_t idx, std::istream &in,
                            PinyinDictFormat format) {
    switch (format) {
    case PinyinDictFormat::Text:
        loadText(idx, in);
        break;
    case PinyinDictFormat::Binary:
        loadBinary(idx, in);
        break;
    default:
        throw std::invalid_argument("invalid format type");
    }
}

void PinyinDictionary::loadText(size_t idx, std::istream &in) {
    FCITX_D();
    auto &trie = d->tries_[idx];
    trie.clear();

    std::string buf;
    auto isSpaceCheck = boost::is_any_of(" \n\t\r\v\f");
    while (!in.eof()) {
        if (!std::getline(in, buf)) {
            break;
        }

        boost::trim_if(buf, isSpaceCheck);
        std::vector<std::string> tokens;
        boost::split(tokens, buf, isSpaceCheck);
        if (tokens.size() == 3) {
            const std::string &hanzi = tokens[0];
            boost::string_view pinyin = tokens[1];
            float prob = std::stof(tokens[2]);
            auto result = PinyinEncoder::encodeFullPinyin(pinyin.to_string());
            result.insert(result.end(), hanzi.begin(), hanzi.end());
            trie.set(result.data(), result.size(), prob);
        }
    }
}

void PinyinDictionary::loadBinary(size_t idx, std::istream &in) {
    FCITX_D();
    d->tries_[idx].load(in);
}

void PinyinDictionary::save(size_t idx, const char *filename) {
    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    throw_if_io_fail(fout);
    save(idx, fout);
}

void PinyinDictionary::save(size_t idx, std::ostream &out) {
    FCITX_D();
    d->tries_[idx].save(out);
}

void PinyinDictionary::dump(size_t idx, std::ostream &out) {
    FCITX_D();
    std::string buf;
    std::ios state(nullptr);
    state.copyfmt(out);
    auto &trie = d->tries_[idx];
    trie.foreach ([this, &trie, &buf, &out](float value, size_t _len,
                                            PinyinTrie::position_type pos) {
        trie.suffix(buf, _len, pos);
        auto sep = buf.find(PinyinEncoder::initialFinalSepartor);
        boost::string_view ref(buf);
        auto fullPinyin =
            PinyinEncoder::decodeFullPinyin(ref.data(), sep * 2 + 1);
        out << ref.substr(sep * 2 + 1) << " " << fullPinyin << " "
            << std::setprecision(16) << value << std::endl;
        return true;
    });
    out.copyfmt(state);
}

void PinyinDictionary::remove(size_t idx) {
    FCITX_D();
    if (idx <= UserDict) {
        throw std::invalid_argument("User Dict not allow to be removed");
    }
    d->tries_.erase(d->tries_.begin() + idx);
}

size_t PinyinDictionary::dictSize() const {
    FCITX_D();
    return d->tries_.size();
}

void PinyinDictionary::addWord(size_t idx, boost::string_view fullPinyin,
                               boost::string_view hanzi, float cost) {
    FCITX_D();
    auto result = PinyinEncoder::encodeFullPinyin(fullPinyin);
    result.insert(result.end(), hanzi.begin(), hanzi.end());
    d->tries_[idx].set(result.data(), result.size(), cost);
}

void PinyinDictionary::setFuzzyFlags(PinyinFuzzyFlags flags) {
    FCITX_D();
    d->flags_ = flags;
}
}
