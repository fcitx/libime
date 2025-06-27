/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _LIBIME_LIBIME_CORE_UTILS_P_H_
#define _LIBIME_LIBIME_CORE_UTILS_P_H_

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include "endian_p.h"

namespace libime {

template <typename T>
inline T loadNative(const char *data) {
    T v;
    memcpy(&v, data, sizeof(v));
    return v;
}

template <typename T>
inline void storeNative(char *data, T v) {
    memcpy(data, &v, sizeof(v));
}

// These are only used by DATrie, and it's le32 due to historic reason.
// Marshall* still use be.
template <typename T>
inline T loadDWord(const char *data) {
    static_assert(sizeof(T) == sizeof(uint32_t), "");
    union {
        uint32_t i;
        T v;
    } c;
    memcpy(&c, data, sizeof(c));
    c.i = le32toh(c.i);
    return c.v;
}

template <typename T>
inline void storeDWord(char *data, T v) {
    static_assert(sizeof(T) == sizeof(uint32_t), "");
    union {
        uint32_t i;
        T v;
    } c;
    c.v = v;
    c.i = htole32(c.i);
    memcpy(data, &c, sizeof(c));
}

static inline std::istream &unmarshallLE(std::istream &in, uint32_t &data) {
    in.read(reinterpret_cast<char *>(&data), sizeof(data));
    data = le32toh(data);
    return in;
}

static inline std::istream &unmarshallVector(std::istream &in,
                                             std::vector<char> &data) {
    in.read(reinterpret_cast<char *>(data.data()), sizeof(char) * data.size());
    return in;
}

template <typename T>
std::ostream &marshall(std::ostream &out, T data)
    requires(sizeof(T) == sizeof(uint32_t))
{
    union {
        uint32_t i;
        T v;
    } c;
    static_assert(sizeof(T) == sizeof(uint32_t),
                  "this function is only for 4 byte data");
    c.v = data;
    c.i = htobe32(c.i);
    return out.write(reinterpret_cast<char *>(&c.i), sizeof(c.i));
}

template <typename T>
std::ostream &marshall(std::ostream &out, T data)
    requires(sizeof(T) == sizeof(uint8_t))
{
    return out.write(reinterpret_cast<char *>(&data), sizeof(data));
}

template <typename T>
std::ostream &marshall(std::ostream &out, T data)
    requires(sizeof(T) == sizeof(uint16_t))
{
    union {
        uint16_t i;
        T v;
    } c;
    static_assert(sizeof(T) == sizeof(uint16_t),
                  "this function is only for 2 byte data");
    c.v = data;
    c.i = htobe16(c.i);
    return out.write(reinterpret_cast<char *>(&c.i), sizeof(c.i));
}

template <typename T>
std::istream &unmarshall(std::istream &in, T &data)
    requires(sizeof(T) == sizeof(uint32_t))
{
    union {
        uint32_t i;
        T v;
    } c;
    static_assert(sizeof(T) == sizeof(uint32_t),
                  "this function is only for 4 byte data");
    if (in.read(reinterpret_cast<char *>(&c.i), sizeof(c.i))) {
        c.i = be32toh(c.i);
        data = c.v;
    }
    return in;
}

template <typename T>
std::istream &unmarshall(std::istream &in, T &data)
    requires(sizeof(T) == sizeof(uint8_t))
{
    return in.read(reinterpret_cast<char *>(&data), sizeof(data));
}

template <typename T>
std::istream &unmarshall(std::istream &in, T &data)
    requires(sizeof(T) == sizeof(uint16_t))
{
    union {
        uint16_t i;
        T v;
    } c;
    static_assert(sizeof(T) == sizeof(uint16_t),
                  "this function is only for 2 byte data");
    if (in.read(reinterpret_cast<char *>(&c.i), sizeof(c.i))) {
        c.i = be16toh(c.i);
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
        if (!out.write(str.data(), str.size())) {
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

template <typename T>
inline int millisecondsTill(T t0) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - t0)
        .count();
}

} // namespace libime

#endif // _LIBIME_LIBIME_CORE_UTILS_P_H_
