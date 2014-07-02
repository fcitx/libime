#ifndef LIBIME_STRINGENCODER_H
#define LIBIME_STRINGENCODER_H
#include <string>
#include "common.h"

namespace libime {

    class StringEncoder
    {
    public:
        virtual void encode(const std::string& input, Bytes& bytes) = 0;
    };
}

#endif // LIBIME_STRINGENCODER_H
