#include "loudstrie.h"
#include "smallvector.h"
#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <boost/iterator/iterator_concepts.hpp>

namespace libime
{

template <class T>
class block_ostream_iterator :
    public std::iterator<std::output_iterator_tag, void, void, void, void>
{
    std::ostream* out_stream;

public:
    block_ostream_iterator(std::ostream& s) : out_stream(&s) {}
    ~block_ostream_iterator() {}

    block_ostream_iterator<T>& operator= (const T& value) {
        out_stream->write(reinterpret_cast<const char*>(&value), sizeof(T));
        return *this;
    }

    block_ostream_iterator<T>& operator*() { return *this; }
    block_ostream_iterator<T>& operator++() { return *this; }
    block_ostream_iterator<T>& operator++(int) { return *this; }
};

struct LoudsTrieBuilder::Private
{
    std::vector<smallvector> words;
    std::vector<int> ids;
};

LoudsTrieBuilder::LoudsTrieBuilder() :
    d(new Private)
{

}

LoudsTrieBuilder::~LoudsTrieBuilder()
{

}

void LoudsTrieBuilder::add(const uint8_t* begin, const uint8_t* end)
{
    d->words.push_back(smallvector(begin, end));
}

void LoudsTrieBuilder::build(std::ostream& out)
{
    std::sort(d->words.begin(), d->words.end());
    d->words.erase(std::unique(d->words.begin(), d->words.end()), d->words.end());

    std::vector<size_t> indices;
    indices.reserve(d->words.size());
    for (size_t idx = 0; idx < d->words.size(); idx++) {
        indices.push_back(idx);
    }

    d->ids.resize(d->words.size(), -1);

    boost::dynamic_bitset<uint32_t> trie_stream;
    boost::dynamic_bitset<uint32_t> terminal_stream;
    smallvector edge_character;

    trie_stream.push_back(1);
    trie_stream.push_back(0);
    edge_character.push_back(0);
    terminal_stream.push_back(0);

    int id = 0;
    for (size_t depth = 0; !indices.empty(); ++depth) {
        for (size_t i = 0; i < indices.size(); ++i) {
            const smallvector &word = d->words[indices[i]];
            if (word.size() > depth &&
                (i == 0 || d->words[indices[i - 1]].size() < depth + 1 ||
                !std::equal(word.begin(), word.end(), d->words[indices[i - 1]].begin()))) {
                // This is the first string of this node. Output an edge.
                trie_stream.push_back(1);
                edge_character.push_back(word[depth]);

                if (word.size() == depth + 1) {
                    // This is a terminal node.
                    // Note that the terminal string should be at the first of
                    // strings sharing the node. So the check above should work well.
                    terminal_stream.push_back(1);
                    d->ids[indices[i]] = id;
                    ++id;
                } else {
                    // This is not a terminal node.
                    terminal_stream.push_back(0);
                }
            }

            if (i == d->words.size() - 1 ||
                !std::equal(word.begin(), word.begin() + depth, d->words[indices[i + 1]].begin())) {
                // This is the last child (string) for the parent.
                trie_stream.push_back(0);
            }
        }

        // Remove all terminal strings.
        indices.erase(
            std::remove_if(
                indices.begin(),
                indices.end(),
                [&](size_t idx) {
                    return d->words[idx].size() < depth + 1;
                }
                ),
            indices.end());
    }

    while (trie_stream.size() % 32) {
        trie_stream.push_back(0);
    }
    while (terminal_stream.size() % 32) {
        terminal_stream.push_back(0);
    }

    uint32_t s;
    s = trie_stream.size() / 8;
    out.write(reinterpret_cast<char*>(&s), sizeof(s));
    s = terminal_stream.size() / 8;
    out.write(reinterpret_cast<char*>(&s), sizeof(s));
    s = edge_character.size();
    out.write(reinterpret_cast<char*>(&s), sizeof(s));
    boost::to_block_range(trie_stream, block_ostream_iterator<uint32_t>(out));
    boost::to_block_range(terminal_stream, block_ostream_iterator<uint32_t>(out));
}

struct LoudsTrie::Private
{
    Private() {
        ;
    }

    const uint8_t* edge_character;

    int rank0(int n) {
        return n - rank1(n);
    }

    int rank1(int n) {

    }
    int select0(int n);
    int select1(int n);
};

LoudsTrie::LoudsTrie(const uint8_t* data, size_t length)
{
    const uint32_t trie_size = *reinterpret_cast<uint32_t*>(data);
    const uint32_t terminal_size = *reinterpret_cast<uint32_t*>(data + 4);
    const uint32_t edge_character_size = *reinterpret_cast<uint32_t*>(data + 8);

    const uint8_t* trie_data = data + 12;
    const uint8_t* terminal_data = trie_data + trie_size;
    d->edge_character = terminal_data + terminal_size;

}



}
