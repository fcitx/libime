/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _TEST_TESTUTILS_H_
#define _TEST_TESTUTILS_H_

#include <chrono>

struct ScopedNanoTimer {
    std::chrono::high_resolution_clock::time_point t0;
    std::function<void(int64_t)> cb;

    ScopedNanoTimer(std::function<void(int64_t)> callback)
        : t0(std::chrono::high_resolution_clock::now()),
          cb(std::move(callback)) {}
    ~ScopedNanoTimer(void) {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto nanos =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0)
                .count();

        cb(nanos);
    }
};

#endif // _TEST_TESTUTILS_H_
