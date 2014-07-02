#ifndef LIBIME_DECODER_H
#define LIBIME_DECODER_H
#include <string>
#include <vector>
#include <memory>
#include "languagemodel.h"

namespace libime
{
    class Segment {};
    class Lattice {};
    class Decoder
    {
        void decode(const std::vector<std::string>& input,
                    int nbest,
                    int constrains[],
                    std::vector<Segment>& segement);
        void buildLattices(const std::vector< std::string >& input);

        std::vector<std::vector<Lattice> > lattices;
        std::shared_ptr<LanguageModel> model;
    };
}

#endif // LIBIME_DECODER_H
