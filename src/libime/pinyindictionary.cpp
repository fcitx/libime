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
#include <boost/utility/string_view.hpp>
#include <cmath>
#include <fstream>
#include <iostream>

namespace libime {

class PinyinDictionaryPrivate {
public:
    DATrie<float> trie;
};

void PinyinDictionary::matchWords(const char *data, size_t size, MatchCallback callback) {
    if (size % 2 != 1 || data[size / 2] != PinyinEncoder::initialFinalSepartor) {
        throw std::invalid_argument("invalid pinyin");
    }

    return matchWords(data, data + size / 2 + 1, size / 2, callback);
}

void PinyinDictionary::matchWords(const char *initials, const char *finals, size_t size, MatchCallback callback) {
    FCITX_D();
    for (size_t i = 0; i < size; i++) {
        if (!PinyinEncoder::isValidInitial(initials[i])) {
            throw std::invalid_argument("invalid pinyin");
        }
    }

    std::list<decltype(d->trie)::position_type> nodes;
    nodes.emplace_back(0);
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
            if (current != PinyinEncoder::wildcard) {
                decltype(d->trie)::value_type result;
                result = d->trie.traverse(&current, 1, *iter);

                if (decltype(d->trie)::isNoPath(result)) {
                    nodes.erase(iter++);
                } else {
                    iter++;
                }
            } else {
                bool changed = false;
                for (char test = PinyinEncoder::firstFinal; test <= PinyinEncoder::lastFinal; test++) {
                    decltype(extraNodes)::value_type p = *iter;
                    auto result = d->trie.traverse(&test, 1, p);
                    if (!decltype(d->trie)::isNoPath(result)) {
                        extraNodes.push_back(p);
                        changed = true;
                    }
                }
                if (changed) {
                    *iter = extraNodes.back();
                    extraNodes.pop_back();
                } else {
                    nodes.erase(iter++);
                }
            }
        }
        nodes.splice(nodes.end(), std::move(extraNodes));
    }

    for (auto node : nodes) {
        d->trie.foreach (
            [d, &callback, size](decltype(d->trie)::value_type value, size_t len, uint64_t pos) {
                std::string s;
                d->trie.suffix(s, len + size * 2 + 1, pos);
                return callback(s.c_str(), s.substr(size * 2 + 1), value);
            },
            node);
    }
}

PinyinDictionary::PinyinDictionary(const char *filename, PinyinDictFormat format)
    : d_ptr(std::make_unique<PinyinDictionaryPrivate>()) {
    std::ifstream in(filename, std::ios::in);
    throw_if_io_fail(in);
    switch (format) {
    case PinyinDictFormat::Text:
        build(in);
        break;
    case PinyinDictFormat::Binary:
        open(in);
        break;
    default:
        throw std::invalid_argument("invalid format type");
    }
}

PinyinDictionary::~PinyinDictionary() {}

void PinyinDictionary::build(std::ifstream &in) {
    FCITX_D();

    std::string buf;
    auto isSpaceCheck = boost::is_any_of(" \n\t\r\v\f");
    while (!in.eof()) {
        if (!std::getline(in, buf)) {
            break;
        }

        boost::trim_if(buf, isSpaceCheck);
        std::vector<std::string> tokens;
        boost::split(tokens, buf, boost::is_any_of(" "));
        if (tokens.size() >= 3) {
            const std::string &hanzi = tokens[0];
            for (size_t i = 2; i < tokens.size(); i++) {
                auto colon = tokens[i].find(':');
                float prob;

                boost::string_view pinyin;
                if (colon == std::string::npos) {
                    prob = std::log10(1.0f / (tokens.size() - 2));
                    pinyin = tokens[i];
                } else {
                    prob = std::log10(std::stof(tokens[i].substr(colon + 1, tokens[i].size() - colon - 2)) / 100);
                    pinyin = boost::string_view(tokens[i]).substr(0, colon);
                }
                auto result = PinyinEncoder::encodeFullPinyin(pinyin.to_string());
                result.insert(result.end(), hanzi.begin(), hanzi.end());
                d->trie.set(result.data(), result.size(), prob);
            }
        }
    }
}

void PinyinDictionary::open(std::ifstream &in) {
    FCITX_D();
    d->trie = decltype(d->trie)(in);
}

void PinyinDictionary::save(const char *filename) {
    std::ofstream fout(filename, std::ios::out | std::ios::binary);
    throw_if_io_fail(fout);
    save(fout);
}

void PinyinDictionary::save(std::ostream &out) {
    FCITX_D();
    d->trie.save(out);
}

void PinyinDictionary::dump(std::ostream &out) {
    FCITX_D();
    std::string buf;
    d->trie.foreach ([this, d, &buf, &out](int32_t, size_t _len, DATrie<int32_t>::position_type pos) {
        d->trie.suffix(buf, _len, pos);
        auto sep = buf.find(PinyinEncoder::initialFinalSepartor);
        boost::string_view ref(buf);
        auto fullPinyin = PinyinEncoder::decodeFullPinyin(ref.data(), sep * 2 + 1);
        out << fullPinyin << " " << ref.substr(sep * 2 + 1) << " " << buf << std::endl;
        return true;
    });
}
}
