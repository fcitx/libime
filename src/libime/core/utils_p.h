/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _LIBIME_LIBIME_CORE_UTILS_P_H_
#define _LIBIME_LIBIME_CORE_UTILS_P_H_

#include <cstdint>
#include <iostream>
#include <vector>

#if defined(__linux__) || defined(__GLIBC__) || defined(__EMSCRIPTEN__)
#include <endian.h>
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#else
#include <sys/endian.h>
#endif

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

} // namespace libime

#endif // _LIBIME_LIBIME_CORE_UTILS_P_H_
