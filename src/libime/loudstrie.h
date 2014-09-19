#ifndef LIBIME_LOUDSTRIE_H
#define LIBIME_LOUDSTRIE_H
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <memory>
#include "common.h"

namespace libime
{

    class LoudsTrie
    {
        class Private;
    public:
        typedef uint8_t key_type;
        typedef int value_type;
        LoudsTrie(const uint8_t* data, size_t length);
        ~LoudsTrie();

        template<typename _Visitor>
        void foreach(_Visitor& visitor)
        {
            prefix_foreach(static_cast<key_type*>(nullptr), static_cast<key_type*>(nullptr), visitor);
        }


        template<typename Container, typename _Visitor>
        void prefix_foreach(const Container& container, _Visitor& visitor)
        {
            prefix_foreach(container.begin(), container.end(), visitor);
        }

        template<typename _Iter, typename _Visitor>
        void prefix_foreach(_Iter begin, _Iter end, _Visitor& visitor)
        {
        }

        template<typename _Iter>
        value_type* find(_Iter begin, _Iter end)
        {
            int node_id = 1;
            int bit_index = 2;
            for (auto iter = begin; iter != end; end++) {
                node_id = get_child_node_id(bit_index);
                while (true) {
                    if (is_edge_bit(bit_index)) {
                        return nullptr;
                    }
                }
                if (edge_character[node_id - 1] == *iter) {
                    bit_index = get_first_edge_bit_index(node_id);
                    break;
                }
                node_id++;
                bit_index++;
            }
            return terminal_bit_vector->get(node_id - 1)
        }
    private:
        std::unique_ptr<Private> d;
    };

    class LIBIME_API LoudsTrieBuilder {
        class Private;
    public:
        LoudsTrieBuilder();
        ~LoudsTrieBuilder();

        void add(const char* begin, const char* end) {
            add(reinterpret_cast<const uint8_t*>(begin), reinterpret_cast<const uint8_t*>(end));
        }
        void add(const uint8_t* begin, const uint8_t* end);

        void build(std::ostream& out);

    private:
        std::unique_ptr<Private> d;
    };
}

#endif // LIBIME_LOUDSTRIE_H
