/*
 * Copyright (C) 2015~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBIME_UTILS_H
#define LIBIME_UTILS_H

#include <arpa/inet.h>
#include <cstdint>
#include <iostream>
#include <vector>

namespace libime {

template <typename T>
inline T load_data(const char *data) {
#if defined(__arm) || defined(__arm__) || defined(__sparc)
    T v;
    memcpy(&v, data, sizeof(v));
    return v;
#else
    return *reinterpret_cast<const T *>(data);
#endif
}

template <typename T>
inline void store_data(char *data, T v) {
#if defined(__arm) || defined(__arm__) || defined(__sparc)
    memcpy(data, &v, sizeof(v));
#else
    *reinterpret_cast<T *>(data) = v;
#endif
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), std::ostream &>::type
marshall(std::ostream &out, T data) {
    union {
        uint32_t i;
        T v;
    } c;
    static_assert(sizeof(T) == sizeof(uint32_t),
                  "this function is only for 4 byte data");
    c.v = data;
    c.i = htonl(c.i);
    return out.write(reinterpret_cast<char *>(&c.i), sizeof(c.i));
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint8_t), std::ostream &>::type
marshall(std::ostream &out, T data) {
    return out.write(reinterpret_cast<char *>(&data), sizeof(data));
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), std::ostream &>::type
marshall(std::ostream &out, T data) {
    union {
        uint16_t i;
        T v;
    } c;
    static_assert(sizeof(T) == sizeof(uint16_t),
                  "this function is only for 2 byte data");
    c.v = data;
    c.i = htons(c.i);
    return out.write(reinterpret_cast<char *>(&c.i), sizeof(c.i));
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), std::istream &>::type
unmarshall(std::istream &in, T &data) {
    union {
        uint32_t i;
        T v;
    } c;
    static_assert(sizeof(T) == sizeof(uint32_t),
                  "this function is only for 4 byte data");
    if (in.read(reinterpret_cast<char *>(&c.i), sizeof(c.i))) {
        c.i = ntohl(c.i);
        data = c.v;
    }
    return in;
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint8_t), std::istream &>::type
unmarshall(std::istream &in, T &data) {
    return in.read(reinterpret_cast<char *>(&data), sizeof(data));
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), std::istream &>::type
unmarshall(std::istream &in, T &data) {
    union {
        uint16_t i;
        T v;
    } c;
    static_assert(sizeof(T) == sizeof(uint16_t),
                  "this function is only for 4 byte data");
    if (in.read(reinterpret_cast<char *>(&c.i), sizeof(c.i))) {
        c.i = ntohs(c.i);
        data = c.v;
    }
    return in;
}

template <typename E>
void throw_if_fail(bool fail, E &&e) {
    if (fail) {
        throw e;
    }
}

inline void throw_if_io_fail(const std::ios &s) {
    throw_if_fail(!s, std::ios_base::failure("io fail"));
}

/// \brief Helper function compare length. If limit is less than 0, it means no
// limit. Avoid unsigned / signed compare.
inline bool lengthLessThanLimit(size_t length, int limit) {
    if (limit < 0) {
        return 0;
    }
    return length < static_cast<size_t>(limit);
}
}

#endif // LIBIME_UTILS_H
