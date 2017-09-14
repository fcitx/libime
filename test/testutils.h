/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _TEST_TESTUTILS_H_
#define _TEST_TESTUTILS_H_

#include <chrono>

struct ScopedNanoTimer {
    std::chrono::high_resolution_clock::time_point t0;
    std::function<void(int)> cb;

    ScopedNanoTimer(std::function<void(int)> callback)
        : t0(std::chrono::high_resolution_clock::now()), cb(callback) {}
    ~ScopedNanoTimer(void) {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto nanos =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0)
                .count();

        cb(nanos);
    }
};

#endif // _TEST_TESTUTILS_H_
