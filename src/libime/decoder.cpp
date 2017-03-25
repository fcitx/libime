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

#include "decoder.h"
#include "datrie.h"
#include "languagemodel.h"
#include "lm/model.hh"
#include "segments.h"
#include <boost/ptr_container/ptr_vector.hpp>
#include <limits>
#include <memory>

namespace libime {

class LatticeNode {
public:
    LatticeNode(LanguageModel *model, boost::string_view word, size_t from, float cost = 0)
        : word_(word.to_string()), from_(from), cost_(cost), state_(model->nullState()) {}

    std::string word_;
    size_t from_;
    float cost_;
    float score_ = 0.0f;
    State state_;
    LatticeNode *prev_ = nullptr;
};

class DecoderPrivate {
public:
    DecoderPrivate(Dictionary *dict, LanguageModel *model) : dict_(dict), model_(model) {}

    std::vector<boost::ptr_vector<LatticeNode>> buildLattice(const Segments &input,
                                                             const std::vector<int> &constrains) {
        std::vector<boost::ptr_vector<LatticeNode>> lattice;
        lattice.resize(input.size() + 2);

        lattice[0].push_back(new LatticeNode(model_, "", 0));
        lattice[0][0].state_ = model_->beginState();
        lattice[input.size() + 1].push_back(new LatticeNode(model_, "", input.size()));

        for (size_t i = 0; i < input.size(); i++) {
            dict_->matchPrefix(input, i, [this, i, &input, &lattice](
                                             size_t to, boost::string_view entry, float adjust) {
                // FIXME
                if (model_->index(entry) == model_->unknown()) {
                    return;
                }
                lattice[to + 1].push_back(new LatticeNode(model_, entry, i, adjust));
            });
        }
        return lattice;
    }

    Dictionary *dict_;
    LanguageModel *model_;
};

Decoder::Decoder(Dictionary *dict, LanguageModel *model)
    : d_ptr(std::make_unique<DecoderPrivate>(dict, model)) {}

Decoder::~Decoder() {}

void Decoder::decode(const Segments &input, int nbest, const std::vector<int> &constrains) {
    return decode(input, nbest, constrains, std::numeric_limits<double>::max(),
                  -std::numeric_limits<double>::max());
}

void Decoder::decode(const Segments &input, int nbest, const std::vector<int> &constrains,
                     double max, double min) {
    FCITX_D();
    auto lattice = d->buildLattice(input, constrains);
    for (size_t i = 1; i < lattice.size(); i++) {
        for (auto &node : lattice[i]) {
            size_t from = node.from_;
            float max = -std::numeric_limits<float>::max();
            State maxState = d->model_->nullState();
            LatticeNode *maxNode = nullptr;
            for (auto &parent : lattice[from]) {
                State state = d->model_->nullState();
                WordIndex idx;
                if (node.word_.empty()) {
                    idx = d->model_->endSentence();
                } else {
                    idx = d->model_->index(node.word_);
                }
                auto score = parent.score_ + d->model_->score(parent.state_, idx, state);
                if (score > max) {
                    max = score;
                    maxNode = &parent;
                    maxState = std::move(state);
                }
            }

            node.score_ = max + node.cost_;
            node.state_ = std::move(maxState);
            node.prev_ = maxNode;
            {
                auto pos = &node;
                auto bos = &lattice.front()[0];
                std::string sentence;
                while (pos != bos) {
                    sentence = pos->word_ + " " + sentence;
                    pos = pos->prev_;
                }
                std::cout << sentence << " " << node.score_ << std::endl;
            }
        }
    }

    if (nbest == 1) {
        auto pos = &lattice.back()[0];
        auto bos = &lattice.front()[0];
        std::string sentence;
        while (pos != bos) {
            sentence = pos->word_ + " " + sentence;
            pos = pos->prev_;
        }
        std::cout << sentence << std::endl;
    }
}
}
