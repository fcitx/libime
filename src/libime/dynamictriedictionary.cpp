#include "dynamictriedictionary.h"
#include "dynamictrie_impl.h"
#include "stringencoder.h"

namespace libime
{
struct DynamicTrieDictionary::Private {
    char delim;
    char linedelim;
    DynamicTrie<std::string> trie;
    std::shared_ptr< StringEncoder > encoder;
};

DynamicTrieDictionary::DynamicTrieDictionary(char delim, char linedelim) : Dictionary(), d(new Private)
{
    d->delim = delim;
    d->linedelim = linedelim;
}

void DynamicTrieDictionary::setStringEncoder(const std::shared_ptr< StringEncoder >& encoder)
{
    d->encoder = encoder;
}

void DynamicTrieDictionary::read(std::istream& in)
{
    std::string line;
    while (std::getline(in, line, d->linedelim)) {
        size_t pos;
        if ((pos = line.find(line, d->delim)) == std::string::npos) {
            continue;
        }

        if (*line.rbegin() == d->linedelim) {
            line.pop_back();
        }

        if (pos == 0) {
            continue;
        }
        if (pos + 1 == line.size()) {
            continue;
        }

        insert(line.substr(0, pos), line.substr(pos + 1));
    }
}

void DynamicTrieDictionary::write(std::ostream& out)
{

}

void DynamicTrieDictionary::lookup(const std::string& input, std::vector< std::string >& output)
{
    auto inserter = std::back_inserter(output);

    Bytes bytes;
    const uint8_t* data;
    size_t len;

    if (auto encoder = d->encoder.get()) {
        encoder->encode(input, bytes);
        data = bytes.data();
        len = bytes.size();
    } else {
        data = reinterpret_cast<const uint8_t*>(input.c_str());
        len = input.size();
    }

    auto result = d->trie.match(data, len);
    if (result != d->trie.end()) {
        *inserter = *result;
    }
}

void DynamicTrieDictionary::insert(const std::string& input, const std::string& output)
{

}


}
