#ifndef LIBIME_SMALLVECTOR_H
#define LIBIME_SMALLVECTOR_H

#include <cstdlib>
#include <string.h>
#include <malloc.h>
#include <stdint.h>
#include <algorithm>

struct smallvectordata;

const size_t smallvectorsize = sizeof(smallvectordata*);

struct smallvectordata
{
    uint32_t capacity;
    uint8_t ptr[smallvectorsize * 2];
};

union smallvectorunion {
    uint8_t raw[smallvectorsize];
    smallvectordata* data;
};

static_assert(sizeof(smallvectorunion) == sizeof(smallvectorsize), "this should not happen");

class smallvector {
    smallvectorunion _data;
    uint32_t _size;

public:
    typedef uint8_t* iterator;
    typedef const uint8_t* const_iterator;

    template<typename _Iter>
    smallvector(_Iter _begin, _Iter _end) : _size(0)
    {
        _resize(_end - _begin);
        for (uint32_t i = 0; _begin != _end; i++, _begin++) {
            begin()[i] = *_begin;
        }
    }

    smallvector() noexcept : _size(0) {
    }

    smallvector(smallvector&& other)noexcept : _size(other._size)
    {
        _data = other._data;

        other._size = 0;
    }

    smallvector(const smallvector& other) : _size(other._size)
    {
        if (other.is_small()) {
            _data = other._data;
        } else {
            auto s = malloc_size(other.capacity());
            _data.data = reinterpret_cast<smallvectordata*>(malloc(s));
            memcpy(_data.data, other._data.data, s);
        }
    }

    ~smallvector() {
        if (!is_small()) {
            free(_data.data);
        }
    }

    smallvector& operator=(smallvector&& other)
    {
        _size = other._size;
        _data = other._data;

        other._size = 0;
        return *this;
    }

    smallvector& operator=(const smallvector& other)
    {
        _size = other._size;
        if (other.is_small()) {
            _data = other._data;
        } else {
            auto s = malloc_size(other.capacity());
            _data.data = reinterpret_cast<smallvectordata*>(malloc(s));
            memcpy(_data.data, other._data.data, s);
        }
        return *this;
    }

    bool is_small() const {
        return _size <= smallvectorsize;
    }

    uint8_t& operator[](size_t index) {
        return begin()[index];
    }

    const uint8_t& operator[](size_t index) const {
        return begin()[index];
    }

    iterator begin()
    {
        if (is_small()) {
            return _data.raw;
        } else {
            return _data.data->ptr;
        }
    }

    const_iterator begin() const
    {
        if (is_small()) {
            return _data.raw;
        } else {
            return _data.data->ptr;
        }
    }

    iterator end()
    {
        return begin() + _size;
    }

    const_iterator end() const
    {
        return begin() + _size;
    }

    void push_back(uint8_t b) {
        _resize(_size + 1);
        begin()[_size - 1] = b;
    }

    uint32_t size() const {
        return _size;
    }

    uint32_t capacity() const {
        if (is_small()) {
            return smallvectorsize;
        } else {
            return _data.data->capacity;
        }
    }

    void pop_back() {
        _resize(_size - 1);
    }

    size_t malloc_size(uint32_t s) {
        return sizeof(smallvectordata) + s * 2 - smallvectorsize * 2;
    }

    uint32_t round(uint32_t s)
    {
        if (s < smallvectorsize * 2) {
            return smallvectorsize * 2;
        } else {
            s--;
            s |= s >> 1;
            s |= s >> 2;
            s |= s >> 4;
            s |= s >> 8;
            s |= s >> 16;
            s++;
            return s;
        }
    }

    void _resize(uint32_t newsize)
    {
        if (_size <= smallvectorsize) {
            if (newsize <= smallvectorsize) {
                // pass
            } else {
                auto capacity = round(newsize);
                auto newdata = reinterpret_cast<smallvectordata*>(malloc(malloc_size(round(newsize))));
                memcpy(newdata->ptr, begin(), _size);
                _data.data = newdata;
                _data.data->capacity = capacity;
            }
        } else {
            // _size > smallvectorsize
            if (newsize <= smallvectorsize) {
                // shrink
                smallvectordata* pdata = _data.data;
                memcpy(_data.raw, _data.data->ptr, newsize);
                free(pdata);
            } else if (newsize > capacity()) {
                auto capacity = round(newsize);
                auto newdata = reinterpret_cast<smallvectordata*>(realloc(_data.data, malloc_size(round(newsize))));
                _data.data = newdata;
                _data.data->capacity = capacity;
            }
        }
        _size = newsize;
    }

    void resize(uint32_t size)
    {
        _resize(size);
    }

    // add a totally dumb function here.
    void shrink_to_fit()
    {
    }

    friend bool operator==(const smallvector& a, const smallvector& b) {
        return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
    }

    friend bool operator<(const smallvector& a, const smallvector& b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }
};


#endif
