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
#include "lm/model.hh"
#include "segment.h"
#include <boost/ptr_container/ptr_vector.hpp>
#include <limits>
#include <memory>

namespace libime {

class LatticeNode {};

class UnigramNode : public LatticeNode {
public:
    UnigramNode(lm::WordIndex idx) : idx_(idx) {}

    lm::WordIndex idx_;
};

class DecoderPrivate {
public:
    lm::ngram::Model model;
    DATrie<float> trie;

    std::vector<boost::ptr_vector<LatticeNode>> buildLattice(const Segments &input,
                                                             const std::vector<int> &constrains) {
        std::vector<boost::ptr_vector<LatticeNode>> lattice;
        lattice.resize(input.size() + 2);

        auto &v = model.GetVocabulary();
        lattice[0].push_back(new UnigramNode(v.BeginSentence()));
        lattice[input.size() + 1].push_back(new UnigramNode(v.EndSentence()));

        for (size_t i = 0; i < input.size(); i++) {
            auto _input = input.right(i);
            auto size = input.rightSize(i);
            trie.foreach (_input, size, [](float value, size_t len, uint64_t pos) {
#if 0
                var j = i + entry.input.char_count ();
                if (!check_constraint (constraint, i, j))
                    continue;
                var node = new UnigramTrellisNode (entry, j);
                trellis[j].add (node);
#endif
                return true;
            });
        }
        return lattice;
    }
};

libime::Segment Decoder::decode(const Segments &input, int nbest, const std::vector<int> &constrains) {
    return decode(input, nbest, constrains, std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
}

libime::Segment Decoder::decode(const Segments &input, int nbest, const std::vector<int> &constrains, double max,
                                double min) {
    d_ptr->buildLattice(input, constrains);
}
}
