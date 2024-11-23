/*
 * SPDX-FileCopyrightText: 2015-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef LIBIME_UTILS_H
#define LIBIME_UTILS_H

#include "libimecore_export.h"
#include <arpa/inet.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fcitx-utils/log.h>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace libime {

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

inline std::istream &unmarshallString(std::istream &in, std::string &str) {
    uint32_t length = 0;
    do {
        if (!unmarshall(in, length)) {
            break;
        }
        std::vector<char> buffer;
        buffer.resize(length);
        if (!in.read(buffer.data(), sizeof(char) * length)) {
            break;
        }
        str.clear();
        str.reserve(length);
        str.append(buffer.begin(), buffer.end());
    } while (0);
    return in;
}

inline std::ostream &marshallString(std::ostream &out, std::string_view str) {
    uint32_t length = str.size();
    do {
        if (!marshall(out, length)) {
            break;
        }
        if (!out.write(str.data(), sizeof(char) * length)) {
            break;
        }
    } while (0);
    return out;
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

/// Helper function compare length. If limit is less than 0, it means no
/// limit. Avoid unsigned / signed compare.
inline bool lengthLessThanLimit(size_t length, int limit) {
    if (limit < 0) {
        return false;
    }
    return length < static_cast<size_t>(limit);
}

template <typename T>
inline int millisecondsTill(T t0) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - t0)
        .count();
}

LIBIMECORE_EXPORT FCITX_DECLARE_LOG_CATEGORY(libime_logcategory);
#define LIBIME_DEBUG() FCITX_LOGC(::libime::libime_logcategory, Debug)
#define LIBIME_ERROR() FCITX_LOGC(::libime::libime_logcategory, Error)
} // namespace libime

#endif // LIBIME_UTILS_H
