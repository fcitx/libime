#ifndef LIBIME_RADIX_H
#define LIBIME_RADIX_H

#include <stddef.h>
#include <stdint.h>
#include <list>
#include <algorithm>
#include <stack>
#include <memory>
#include <boost/optional.hpp>
#include "smallvector.h"

namespace libime
{

template<typename _Value, bool _sorted>
struct RadixNode {
    typedef uint8_t key_type;
    typedef smallvector keyvec_type;
    typedef RadixNode<_Value, _sorted> node_type;
    typedef typename keyvec_type::iterator keyiter;
    typedef typename std::list<node_type> nodes_type;
    typedef typename nodes_type::iterator nodeiter;

    RadixNode()
    {
    }

    template<typename _Iter>
    RadixNode(_Iter begin, _Iter end) : key(begin, end)
    {
    }

    template<typename _Iter>
    RadixNode(_Iter begin, _Iter end, _Value&& _data) : key(begin, end), data(std::forward<_Value>(_data))
    {
    }

    RadixNode(RadixNode&& node) :
        key(std::move(node.key)),
        data(std::move(node.data)),
        _subNodes(std::move(node._subNodes))
    {
    }
    RadixNode& operator=(RadixNode&& node)
    {
        key = std::move(node.key);
        data = std::move(node.data);
        _subNodes = std::move(node._subNodes);
        return *this;
    }

    RadixNode(const RadixNode& node) :
        key(node.key),
        data(node.data),
        _subNodes(node._subNodes ? new nodes_type(*node._subNodes) : 0)
    {
    }
    RadixNode& operator=(const RadixNode& node)
    {
        key = node.key;
        data = node.data;
        _subNodes = node._subNodes ? new nodes_type(*node._subNodes) : 0;
        return *this;
    }

    nodeiter begin()
    {
        if (_subNodes) {
            return subNodes()->begin();
        }

        return nodeiter();
    }

    nodeiter end()
    {
        if (_subNodes) {
            return subNodes()->end();
        }

        return nodeiter();
    }

    size_t size()
    {
        if (_subNodes) {
            return subNodes()->size();
        }

        return 0;
    }

    void erase(nodeiter iter)
    {
        subNodes()->erase(iter);
        if (size() == 0) {
            _subNodes.reset();
        }
    }

    template<typename T>
    void addSubNode(T&& node)
    {
        subNodes()->push_back(std::forward<T>(node));/*
        if (_sorted) {
            std::sort(begin(), end());
        }*/
    }

    bool operator <(const RadixNode& node) const {
        return key[0] < node.key[0];
    }

    bool operator <(key_type _key) const {
        return key[0] < _key;
    }

    bool operator==(key_type _key) const {
        return key[0] == _key;
    }

    bool operator!=(key_type _key) const {
        return !operator==(_key);
    }

    nodeiter find(const key_type& t)
    {
        if (_subNodes) {

/*
            if (_sorted) {
                auto iter = std::lower_bound(begin(), end(), t);
                if (iter != end() && *iter == t) {
                    return iter;
                }
                return end();
            }*/
            return std::find(begin(), end(), t);
        }

        return nodeiter();
    }

    keyvec_type key;
    boost::optional<_Value> data;
    std::unique_ptr<nodes_type> _subNodes;

private:
    nodes_type* subNodes()
    {
        if (!_subNodes) {
            _subNodes.reset(new nodes_type);
        }

        return _subNodes.get();
    }
};

template<typename _Key>
struct RadixTreeIterator : std::iterator<std::forward_iterator_tag,
                                              std::pair<std::vector<_Key>, void*>>
{
    std::vector<_Key> key;

    RadixTreeIterator& operator++()
    {

        return (*this);
    }

    RadixTreeIterator operator++(int)
    {
        RadixTreeIterator __tmp = *this;
        ++(*this);
        return __tmp;
    }
};

template<typename _Value, bool _sorted = true>
class RadixTree
{
public:
    typedef uint8_t key_type;
    typedef _Value value_type;
    typedef RadixNode<value_type, _sorted> node_type;
    typedef typename node_type::keyiter keyiter;
    typedef typename node_type::nodeiter nodeiter;
    typedef RadixTreeIterator<key_type> iterator;

    RadixTree() : _size(), _root(node_type()) {
    }

    std::size_t size()
    {
        return _size;
    }

    template<typename Container>
    bool add(const Container& container, const value_type& value)
    {
        return add(container, value_type(value));
    }

    template<typename _Iter>
    bool add(_Iter begin, _Iter end, const value_type& value)
    {
        return add(begin, end, value_type(value));
    }

    template<typename Container>
    bool add(const Container& container, value_type&& value)
    {
        return add(container.begin(), container.end(), std::forward<value_type>(value));
    }

    template<typename _Iter>
    bool add(_Iter begin, _Iter end, value_type&& value)
    {
        if (_add(begin, end, std::forward<value_type>(value))) {
            _size ++;
            return true;
        }
        return false;
    }


    template<typename _Iter>
    nodeiter _find(_Iter keyBegin, _Iter keyEnd, node_type** parent = nullptr, _Iter* keyOff = nullptr, keyiter* nodeKeyOff = nullptr)
    {
        node_type* node = &_root;
        nodeiter subNodeIter;
        keyiter subNodeKey = _root.key.end();
        while (true) {
            subNodeIter = node->find(*keyBegin);
            if (subNodeIter == node->end()) {
                break;
            }

            auto& subNode = *subNodeIter;
            subNodeKey = subNode.key.begin();
            auto subNodeKeyEnd = subNode.key.end();
            while(keyBegin != keyEnd && subNodeKey != subNodeKeyEnd && *keyBegin == *subNodeKey) {
                keyBegin++;
                subNodeKey++;
            }

            if (keyBegin == keyEnd && subNodeKey == subNodeKeyEnd) {
                // exact match
                break;
            }

            node = &subNode;
            subNodeIter = node->end();
            if (subNodeKey != subNodeKeyEnd) {
                // doesn't match
                break;
            }
        }

        if (parent) {
            *parent = node;
        }
        if (keyOff) {
            *keyOff = keyBegin;
        }
        if (nodeKeyOff) {
            *nodeKeyOff = subNodeKey;
        }
        return subNodeIter;
    }

    template<typename _Iter>
    bool _add(_Iter begin, _Iter end, value_type&& data)
    {
        node_type* node = nullptr;
        keyiter nodeKey;
        auto subNodeIter = _find(begin, end, &node, &begin, &nodeKey);
        if (subNodeIter != node->end()) {
            auto& subNode = *subNodeIter;
            if (subNode.data) {
                return false;
            } else {
                subNode.data = std::move(data);
            }
        } else if (node->key.end() == nodeKey) {
            node->addSubNode(std::move(node_type(begin, end, std::move(data))));
            return true;
        } else {
            // split node like aac into aa -> c
            // node will become aa
            node_type branchNode(nodeKey, node->key.end());
            node->key.resize(nodeKey - node->key.begin());
            node->key.shrink_to_fit();
            std::swap(branchNode._subNodes, node->_subNodes);
            if (begin != end) {
                // case like  aac, add aab
                node->addSubNode(std::move(node_type(begin, end, std::move(data))));
            } else {
                // case like  aac, add aa
                branchNode.data.reset(std::move(data));
            }
            node->addSubNode(std::move(branchNode));
        }
        return true;
    }

    template<typename Container>
    bool remove(const Container& container)
    {
        return remove(container.begin(), container.end());
    }

    template<typename _Iter>
    bool remove(_Iter begin, _Iter end)
    {
        node_type* node;
        auto subNodeIter = _find(begin, end, &node);
        auto& subNode = *subNodeIter;

        if (subNodeIter != node->end() && subNode.data) {
            subNode.data.reset();

            // if the dict is a, ab, and remove a.
            if (subNode.size() == 0) {
                node->erase(subNodeIter);

                if (node->size() == 1 && node != &_root && node->data) {
                    _merge(*node);
                }
            } else if (subNode.size() == 1) {
                // if dict contains a, ab, abc, and remove ab.
                _merge(subNode);
            }
            _size --;
            return true;
        }
        return false;
    }

    void _merge(node_type& node)
    {
        auto sub2iter = node.begin();
        auto& sub2 = *sub2iter;

        auto begin = sub2.key.begin();
        auto end = sub2.key.end();
        while(begin != end) {
            node.key.push_back(*begin);
        }
        node.key.shrink_to_fit();
        node.data = std::move(sub2.data);
        node._subNodes = std::move(sub2._subNodes);
    }

    iterator begin()
    {
        iterator iter;
        return iter;
    }

    iterator end()
    {
        return iterator();
    }

    template<typename _Iter>
    iterator prefix(_Iter begin, _Iter end)
    {
        node_type* node = nullptr;
        _Iter keyOff;
        keyiter nodeKey;
        auto subNode = _find(begin, end, &node, &keyOff, &nodeKey);
        if (begin != end) {
            return end();
        }

        iterator iter;
        iter.key.assign(begin, keyOff);
        iter.key.insert(iter.key.end(), nodeKey, node->key.end());

        return iter;
    }

private:
    std::size_t _size;
    node_type _root;
};

}

#endif // LIBIME_RADIX_H
