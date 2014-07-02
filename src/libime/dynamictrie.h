#ifndef LIBIME_DYNAMICTRIE_H
#define LIBIME_DYNAMICTRIE_H
#include "common.h"
#include <algorithm>
#include <array>

namespace libime
{
    template<typename V>
    struct DynamicTrieIterator
    {
        DynamicTrieIterator(V* value=0) { }

        V* value;

        V& operator* ()
        {
            return *value;
        }

        bool operator != (const DynamicTrieIterator<V>& rhs )
        {
            return value != rhs.value;
        }
    };

    template<typename V>
    struct DynamicTrieNode
    {
        Bytes key;
        V value;
        std::vector<DynamicTrieNode<V> > subNodes;

        bool operator < (uint8_t key) {
            return this->key < key;
        }

        bool operator < (const DynamicTrieNode<V>& rhs) {
            return this->key < rhs.key;
        }

        DynamicTrieNode<V>& get(uint8_t key)
        {
            std::lower_bound(subNodes.begin(), subNodes.end(), key);

            return NULL;
        }

        void add(DynamicTrieNode<V>& node)
        {
            subNodes.push_back(node);
            std::sort(subNodes);
        }
    };

    template<typename V>
    class DynamicTrie
    {
    public:
        typedef DynamicTrieIterator<V> iterator;
        typedef DynamicTrieNode<V> node_type;
        typedef typename std::size_t size_type;

        bool add(const uint8_t* key, std::size_t len, const V& value);

        bool remove(const uint8_t* key, std::size_t len);
        iterator match(const uint8_t* key, std::size_t len);
        iterator prefix(const uint8_t* key, std::size_t len);

        size_type size();

        iterator begin();
        iterator end();

    private:
        node_type _root;
        size_type _size;
    };
}

#endif // LIBIME_DYNAMICTRIE_H
