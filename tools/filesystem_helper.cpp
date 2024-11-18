/*
 * SPDX-FileCopyrightText: 2024~2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <string>
#include <string_view>

#ifdef HAS_STD_FILESYSTEM
#include <filesystem>
#else
#include <boost/filesystem.hpp>
#endif

namespace libime {

std::string absolutePath(const std::string &path) {
#ifdef HAS_STD_FILESYSTEM
    return std::filesystem::absolute(path);
#else
    return boost::filesystem::absolute(path).string();
#endif
}

} // namespace libime
