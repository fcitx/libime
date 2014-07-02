#ifndef LIBIME_DICTIONARY_H
#define LIBIME_DICTIONARY_H
#include <string>
#include <vector>

namespace libime
{
    class Dictionary {
    public:
        virtual void lookup(const std::string& input, std::vector<std::string>& output) = 0;
    };
}

#endif // LIBIME_DICTIONARY_H
