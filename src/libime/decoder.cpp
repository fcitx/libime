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

class LatticeNode {};

class UnigramNode : public LatticeNode {
public:
    UnigramNode(WordIndex idx, float cost = 0) : idx_(idx), cost_(cost) {}

    WordIndex idx_;
    float cost_;
};

class DecoderPrivate {
public:
    DecoderPrivate(Dictionary *dict, LanguageModel *model) : dict_(dict), model_(model) {}

    std::vector<boost::ptr_vector<LatticeNode>> buildLattice(const Segments &input,
                                                             const std::vector<int> &constrains) {
        std::vector<boost::ptr_vector<LatticeNode>> lattice;
        lattice.resize(input.size() + 2);

        lattice[0].push_back(new UnigramNode(model_->beginSentence()));
        lattice[input.size() + 1].push_back(new UnigramNode(model_->endSentence()));

        for (size_t i = 0; i < input.size(); i++) {
            auto seg = input.right(i);
            dict_->matchPrefix(
                seg, [this, i, &seg, &lattice](size_t to, boost::string_view entry, float adjust) {
#if 0
                if (!check_constraint (constraint, i, j))
                    return;
#endif
                    lattice[i + to].push_back(new UnigramNode(model_->index(entry), adjust));

                });
        }
        return lattice;
    }

private:
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
    d_ptr->buildLattice(input, constrains);
}
}
