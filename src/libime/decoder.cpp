#include "decoder.h"


void libime::Decoder::buildLattices(const std::vector< std::string >& input)
{
    auto length = input.size();
    lattices.resize(length + 2);
    for (auto lattice : lattices) {
        lattice.clear();
    }

    lattices[0].push_back(Lattice(bos, 1));
    lattices[length + 1][0] = Lattice(Lattice(eos, length + 1));
    for (int i = 0; i < length; i++) {
        auto entries = model->entries (input);
        for (auto entry : entries) {
            int j = i + entry.input.size();/*
            if (!checkConstraint (constraint, i, j)) {
                continue;
            }*/
            lattices[j].add (Lattice(entry, j));
        }
    }
}

void libime::Decoder::decode(const std::vector< std::string >& input, int nbest, int constrains[], std::vector< Segment >& segement)
{
    buildLattices();

}
