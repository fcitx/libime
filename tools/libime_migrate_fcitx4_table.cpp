/*
 * SPDX-FileCopyrightText: 2002~2005 Yuking <yuking_net@sohu.com>
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "config.h"
#include "libime/core/historybigram.h"
#include "libime/core/utils.h"
#include "libime/core/utils_p.h"
#include "libime/table/tablebaseddictionary.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fcitx-utils/charutils.h>
#include <fcitx-utils/fdstreambuf.h>
#include <fcitx-utils/fs.h>
#include <fcitx-utils/standardpaths.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/unixfd.h>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <istream>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace libime;
using namespace fcitx;

struct MigrationCommonOption {
    bool skipHistory = false;
    bool skipDict = false;
    std::string dictFile;
    std::string historyFile;
    bool useXdgPath = true;
    std::string sourceFile;

    UnixFD openSourceFile() const {
        // We support two different mode of migration.
        // For an input method with existing code base.
        UnixFD sourceFd;
        // Fcitx 4's xdg is not following the spec, it's using a xdg_data and
        // xdg_config in mixed way.
        if (useXdgPath && sourceFile[0] != '/') {
            sourceFd = StandardPaths::global().open(
                StandardPathsType::Config,
                std::filesystem::path("fcitx/table") / sourceFile,
                StandardPathsMode::User);
            if (!sourceFd.isValid()) {
                sourceFd = StandardPaths::global().open(
                    StandardPathsType::Data,
                    std::filesystem::path("fcitx/table") / sourceFile,
                    StandardPathsMode::System);
            }
        } else {
            sourceFd = UnixFD::own(open(sourceFile.data(), O_RDONLY));
        }
        return sourceFd;
    }

    UnixFD openMergeFile(const std::string &path) const {
        UnixFD fd;
        if (useXdgPath && path[0] != '/') {
            fd = StandardPaths::global().open(
                StandardPathsType::PkgData,
                std::filesystem::path("table") / path, StandardPathsMode::User);
        } else {
            fd = UnixFD::own(open(path.data(), O_RDONLY));
        }
        return fd;
    }

    std::filesystem::path pathForSave(const std::filesystem::path &path) const {
        if (path.is_relative()) {
            if (useXdgPath) {
                return std::filesystem::path("table") / path;
            }

            return std::filesystem::absolute(path);
        }
        return path;
    }
};

struct MigrationWithBaseOption : public MigrationCommonOption {
    std::string baseFile;
    bool merge = true;
};

struct MigrationWithoutBaseOption : public MigrationCommonOption {};

struct BasicTableInfo {
    std::string code;
    std::string ignoreChars;
    uint32_t length = 0;
    std::string rule;
    char pinyin = '\0';
    char prompt = '\0';
    char phrase = '\0';
};

enum RecordType {
    RECORDTYPE_NORMAL = 0x0,
    RECORDTYPE_PINYIN = 0x1,
    RECORDTYPE_CONSTRUCT = 0x2,
    RECORDTYPE_PROMPT = 0x3,
    RECORDTYPE_LAST = RECORDTYPE_PROMPT,
};

constexpr int INTERNAL_VERSION = 3;
constexpr int MAX_CODE_LENGTH = 60;
constexpr const char mbSuffix[] = ".mb";

std::string_view argv0;

void usage(const char *extra = nullptr) {
    std::cout
        << "Usage: " << argv0
        << " [-o <dict>/-O] [-p <history>/-P] [-b <base>/-B] [-U] [-B] [-X] "
           "<source>"
        << '\n'
        << "<source>: the source file of the dictionary." << '\n'
        << "-o: output dict file path" << '\n'
        << "-O: Skip dict file." << '\n'
        << "-p: history file path" << '\n'
        << "-P: Skip history file" << '\n'
        << "-b: base file of a libime main dict" << '\n'
        << "-B: generate full data without base file" << '\n'
        << "-X: locate non-abstract path by only path instead of Xdg path"
        << '\n'
        << "-U: overwrite instead of merge with existing data." << '\n'
        << "-h: Show this help" << '\n';
    if (extra) {
        std::cout << extra << '\n';
    }
}

std::optional<std::string> replaceSuffix(const std::string &input,
                                         const std::string &suffix,
                                         std::string_view newSuffix) {
    auto name = fs::baseName(input);
    if (!stringutils::endsWith(name, suffix)) {
        return {};
    }
    // Strip .mb
    name.erase(name.size() - suffix.size(), suffix.size());
    name.append(newSuffix);
    return name;
}

char guessValidChar(char prefer, std::string_view invalid) {
    if (invalid.find(prefer) == std::string::npos) {
        return prefer;
    }
    std::string_view punct = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    for (int i = 0; i <= 127; i++) {
        char c = static_cast<char>(i);
        if (punct.find(c) != std::string_view::npos || charutils::isdigit(c) ||
            charutils::islower(c) || charutils::isupper(c)) {
            if (invalid.find(c) == std::string::npos) {
                return c;
            }
        }
    }
    return 0;
}

void loadSource(
    const UnixFD &sourceFd,
    const std::function<void(const BasicTableInfo &info)> &basicInfoCallback,
    const std::function<void(const BasicTableInfo &info, RecordType,
                             const std::string &, const std::string &,
                             uint32_t)> &recordCallback) {
    BasicTableInfo info;
    fcitx::IFDStreamBuf buffer(sourceFd.fd());
    std::istream in(&buffer);

    uint32_t codeStrLength;
    throw_if_io_fail(unmarshallLE(in, codeStrLength));
    // 先读取码表的信息
    bool isOldVersion = 1;
    if (!codeStrLength) {
        uint8_t version;
        throw_if_io_fail(unmarshall(in, version));
        isOldVersion = (version < INTERNAL_VERSION);
        throw_if_io_fail(unmarshallLE(in, codeStrLength));
    }
    std::vector<char> codeString;
    codeString.resize(codeStrLength + 1);
    throw_if_io_fail(unmarshallVector(in, codeString));

    uint8_t len;
    throw_if_io_fail(unmarshall(in, len));
    if (len == 0 || len > MAX_CODE_LENGTH) {
        throw std::runtime_error("Invalid code length");
    }

    uint8_t pylen = 0;
    if (!isOldVersion) {
        throw_if_io_fail(unmarshall(in, pylen));
    }

    info.length = len;
    info.code.assign(codeString.data(), codeStrLength);

    uint32_t invalidCharLength;
    throw_if_io_fail(unmarshallLE(in, invalidCharLength));
    std::vector<char> invalidChar;
    invalidChar.resize(invalidCharLength + 1);
    throw_if_io_fail(unmarshallVector(in, invalidChar));
    info.ignoreChars.assign(invalidChar.data(), invalidCharLength);

    uint8_t hasRule;
    throw_if_io_fail(unmarshall(in, hasRule));
    if (hasRule) {
        std::stringstream ss;
        for (size_t i = 1; i < len; i++) {
            uint8_t ruleFlag;
            throw_if_io_fail(unmarshall(in, ruleFlag));
            ss << (ruleFlag ? 'a' : 'e');
            uint8_t ruleLength;
            throw_if_io_fail(unmarshall(in, ruleLength));
            ss << static_cast<uint32_t>(ruleLength) << '=';

            for (size_t j = 0; j < len; j++) {
                if (j) {
                    ss << '+';
                }
                uint8_t ruleIndex;
                throw_if_io_fail(unmarshall(in, ruleFlag));
                ss << (ruleFlag ? 'p' : 'n');
                throw_if_io_fail(unmarshall(in, ruleIndex));
                ss << static_cast<uint32_t>(ruleIndex);
                throw_if_io_fail(unmarshall(in, ruleIndex));
                ss << static_cast<uint32_t>(ruleIndex);
            }
            ss << '\n';
        }
        info.rule = ss.str();
    }
    std::string invalid = info.code;
    info.pinyin = guessValidChar('@', invalid);
    if (info.pinyin) {
        invalid.push_back(info.pinyin);
    }
    info.prompt = guessValidChar('&', invalid);
    if (info.prompt) {
        invalid.push_back(info.prompt);
    }
    info.phrase = guessValidChar('^', invalid);
    if (info.phrase) {
        invalid.push_back(info.phrase);
    }

    if (basicInfoCallback) {
        basicInfoCallback(info);
    }

    uint32_t nRecords;
    throw_if_io_fail(unmarshallLE(in, nRecords));

    if (!isOldVersion) {
        len = pylen;
    }

    for (size_t i = 0; i < nRecords; i++) {
        std::vector<char> codeBuffer;
        codeBuffer.resize(len + 1);
        throw_if_io_fail(unmarshallVector(in, codeBuffer));
        uint32_t hzLength = 0;
        if (codeBuffer.back() != 0) {
            throw std::runtime_error("Invalid data in source file.");
        }
        throw_if_io_fail(unmarshallLE(in, hzLength));
        std::vector<char> hzBuffer;
        hzBuffer.resize(hzLength);
        throw_if_io_fail(unmarshallVector(in, hzBuffer));
        if (hzLength == 0 || hzBuffer.back() != 0) {
            throw std::runtime_error("Invalid data in source file.");
        }

        uint8_t recordType = RECORDTYPE_NORMAL;
        if (!isOldVersion) {
            throw_if_io_fail(unmarshall(in, recordType));

            if (recordType > RECORDTYPE_LAST) {
                throw std::runtime_error("Invalid record type.");
            }
        }

        uint32_t index;
        uint32_t freq;
        throw_if_io_fail(unmarshallLE(in, freq));
        throw_if_io_fail(unmarshallLE(in, index));

        recordCallback(info, static_cast<RecordType>(recordType),
                       codeBuffer.data(), hzBuffer.data(), freq);
    }
}

int migrate(const MigrationWithBaseOption &option) {
    UnixFD baseFd;
    if (option.baseFile.empty()) {
        if (!option.useXdgPath) {
            usage("Base file name is missing. Please use -b to specifiy the "
                  "base file, or -B to use non-base file mode.");
            return 1;
        }

        if (auto name =
                replaceSuffix(option.sourceFile, mbSuffix, ".main.dict")) {
            baseFd = UnixFD::own(open(
                stringutils::joinPath(LIBIME_INSTALL_PKGDATADIR, *name).data(),
                O_RDONLY));
            if (!baseFd.isValid()) {
                baseFd = StandardPaths::global().open(
                    StandardPathsType::PkgData,
                    std::filesystem::path("table") / *name);
            }
        } else {
            usage("Failed to infer the base file name. Please use -b to "
                  "specifiy the base file, or -B to use non-base file mode.");
            return 1;
        }
    } else {
        baseFd = UnixFD::own(open(option.baseFile.data(), O_RDONLY));
    }
    if (!baseFd.isValid()) {
        usage("Failed to locate base file, please use -b to specifiy the right "
              "base file, or -B to use non-base file mode.");
        return 1;
    }

    std::string dictFile = option.dictFile;
    if (!option.skipDict) {
        if (dictFile.empty()) {
            if (!option.useXdgPath) {
                usage("Output dict file need to be specified.");
                return 1;
            }

            if (auto name =
                    replaceSuffix(option.sourceFile, mbSuffix, ".user.dict")) {
                dictFile = *name;
            } else {
                usage("Failed to infer the dict file name. Please use -o to "
                      "specifiy the dict file, or -O skip.");
                return 1;
            }
        }
    }

    std::string historyFile = option.historyFile;
    if (!option.skipHistory) {
        if (historyFile.empty()) {
            if (!option.useXdgPath) {
                usage("History file need to be specified.");
                return 1;
            }

            if (auto name =
                    replaceSuffix(option.sourceFile, mbSuffix, ".history")) {
                historyFile = *name;
            } else {
                usage("Failed to infer the history file name. Please use -p to "
                      "specifiy the history file, or -P skip.");
                return 1;
            }
        }
    }

    UnixFD sourceFd = option.openSourceFile();
    if (!sourceFd.isValid()) {
        usage("Failed to open the source file.");
        return 1;
    }

    TableBasedDictionary tableDict;
    {
        IFDStreamBuf buffer(baseFd.fd());
        std::istream in(&buffer);
        tableDict.load(in);
    }
    if (option.merge && !option.skipDict) {
        UnixFD dictFd = option.openMergeFile(dictFile);
        if (dictFd.isValid()) {
            try {
                IFDStreamBuf buffer(dictFd.fd());
                std::istream in(&buffer);
                tableDict.loadUser(in);
            } catch (const std::exception &e) {
                std::cout << "Failed when loading dict file: " << e.what();
                return 1;
            }
        }
    }

    HistoryBigram history;
    if (option.merge && !option.skipHistory) {
        UnixFD historyFd = option.openMergeFile(historyFile);
        if (historyFd.isValid()) {
            try {
                IFDStreamBuf buffer(historyFd.fd());
                std::istream in(&buffer);
                history.load(in);
            } catch (const std::exception &e) {
                std::cout << "Failed when loading history file: " << e.what();
                return 1;
            }
        }
    }

    uint32_t mergedWord = 0;
    try {
        loadSource(
            sourceFd, {},
            [&option, &tableDict, &history,
             &mergedWord](const BasicTableInfo &, RecordType type,
                          const std::string &code, const std::string &value,
                          uint32_t freq) {
                if (type != RECORDTYPE_NORMAL) {
                    return;
                }

                if (!option.skipDict) {
                    auto flag = tableDict.wordExists(code, value);
                    if (flag != PhraseFlag::User && flag != PhraseFlag::None) {
                        tableDict.insert(code, value, PhraseFlag::User);
                        ++mergedWord;
                    }
                }

                if (!option.skipHistory) {
                    for (uint32_t i = 0;
                         i < std::min(static_cast<uint32_t>(10U), freq); i++) {
                        history.add({value});
                    }
                }
            });
    } catch (const std::exception &e) {
        std::cout << "Failed when loading source file: " << e.what();
        return 1;
    }

    std::cout << "Found " << mergedWord << " new words." << '\n';
    if (!option.skipDict) {
        if (!StandardPaths::global().safeSave(StandardPathsType::PkgData,
                                              option.pathForSave(dictFile),
                                              [&tableDict](int fd) {
                                                  OFDStreamBuf buffer(fd);
                                                  std::ostream out(&buffer);
                                                  tableDict.saveUser(out);
                                                  return true;
                                              })) {
            std::cout << "Failed to save to dictionary file." << '\n';
            return 1;
        }
    }

    if (!option.skipHistory) {
        if (!StandardPaths::global().safeSave(StandardPathsType::PkgData,
                                              option.pathForSave(historyFile),
                                              [&history](int fd) {
                                                  OFDStreamBuf buffer(fd);
                                                  std::ostream out(&buffer);
                                                  history.save(out);
                                                  return true;
                                              })) {
            std::cout << "Failed to save to history file." << '\n';
            return 1;
        }
    }

    return 0;
}

int migrate(const MigrationWithoutBaseOption &option) {
    std::string dictFile = option.dictFile;
    if (!option.skipDict) {
        if (dictFile.empty()) {
            if (!option.useXdgPath) {
                usage("Output dict file need to be specified.");
                return 1;
            }

            if (auto name =
                    replaceSuffix(option.sourceFile, mbSuffix, ".user.dict")) {
                dictFile = *name;
            } else {
                usage("Failed to infer the dict file name. Please use -o to "
                      "specifiy the dict file, or -O skip.");
                return 1;
            }
        }
    }

    std::string historyFile = option.historyFile;
    if (!option.skipHistory) {
        if (historyFile.empty()) {
            if (!option.useXdgPath) {
                usage("History file need to be specified.");
                return 1;
            }
            if (auto name =
                    replaceSuffix(option.sourceFile, mbSuffix, ".main.dict")) {
                historyFile = *name;
            } else {
                usage("Failed to infer the history file name. Please use -p to "
                      "specifiy the history file, or -P skip.");
                return 1;
            }
        }
    }

    UnixFD sourceFd = option.openSourceFile();
    if (!sourceFd.isValid()) {
        usage("Failed to open the source file.");
        return 1;
    }

    TableBasedDictionary tableDict;
    HistoryBigram history;
    std::stringstream ss;
    try {
        loadSource(
            sourceFd,
            [&ss](const BasicTableInfo &info) {
                ss << "KeyCode=" << info.code << '\n';
                ss << "Length=" << info.length << '\n';

                if (!info.ignoreChars.empty()) {
                    ss << "InvalidChar=" << info.ignoreChars << '\n';
                }

                if (info.pinyin) {
                    ss << "Pinyin=" << info.pinyin << '\n';
                }

                if (info.prompt) {
                    ss << "Prompt=" << info.prompt << '\n';
                }

                if (info.phrase) {
                    ss << "ConstructPhrase=" << info.phrase << '\n';
                }

                if (!info.rule.empty()) {
                    ss << "[Rule]\n" << info.rule << '\n';
                }
                ss << "[Data]" << '\n';
            },
            [&option, &ss, &history](const BasicTableInfo &info,
                                     RecordType type, const std::string &code,
                                     const std::string &value, uint32_t freq) {
                if (!option.skipDict) {
                    switch (type) {
                    case RECORDTYPE_NORMAL:
                        ss << code << " " << value << '\n';
                        break;
                    case RECORDTYPE_CONSTRUCT:
                        if (info.phrase) {
                            ss << info.phrase << code << " " << value << '\n';
                        }
                        break;
                    case RECORDTYPE_PROMPT:
                        if (info.prompt) {
                            ss << info.prompt << code << " " << value << '\n';
                        }
                        break;
                    case RECORDTYPE_PINYIN:
                        if (info.pinyin) {
                            ss << info.pinyin << code << " " << value << '\n';
                        }
                        break;
                    }
                }

                if (!option.skipHistory) {
                    for (uint32_t i = 0;
                         i < std::min(static_cast<uint32_t>(10U), freq); i++) {
                        history.add({value});
                    }
                }
            });
    } catch (const std::exception &e) {
        std::cout << "Failed when loading source file: " << e.what();
        return 1;
    }
    if (!option.skipDict) {
        try {
            tableDict.load(ss, libime::TableFormat::Text);
        } catch (const std::exception &e) {
            std::cout << "Failed when construct new dict: " << e.what();
            return 1;
        }
        if (!StandardPaths::global().safeSave(StandardPathsType::PkgData,
                                              option.pathForSave(dictFile),
                                              [&tableDict](int fd) {
                                                  OFDStreamBuf buffer(fd);
                                                  std::ostream out(&buffer);
                                                  tableDict.save(out);
                                                  return true;
                                              })) {
            std::cout << "Failed to save to dictionary file." << '\n';
            return 1;
        }
    }

    if (!option.skipHistory) {
        if (!StandardPaths::global().safeSave(StandardPathsType::PkgData,
                                              option.pathForSave(historyFile),
                                              [&history](int fd) {
                                                  OFDStreamBuf buffer(fd);
                                                  std::ostream out(&buffer);
                                                  history.save(out);
                                                  return true;
                                              })) {
            std::cout << "Failed to save to history file." << '\n';
            return 1;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    argv0 = argv[0];
    bool noBasefile = false;
    MigrationWithBaseOption withBaseOption;
    MigrationWithoutBaseOption withoutBaseOption;
    int c;
    while ((c = getopt(argc, argv, "Bb:Oo:Pp:UXh")) != -1) {
        switch (c) {
        case 'B':
            noBasefile = true;
            break;
        case 'b':
            withBaseOption.baseFile = optarg;
            break;
        case 'O':
            withBaseOption.skipDict = withoutBaseOption.skipDict = true;
            break;
        case 'o':
            withBaseOption.dictFile = withoutBaseOption.dictFile = optarg;
            break;
        case 'P':
            withBaseOption.skipHistory = withoutBaseOption.skipHistory = true;
            break;
        case 'p':
            withBaseOption.historyFile = withoutBaseOption.historyFile = optarg;
            break;
        case 'U':
            withBaseOption.merge = false;
            break;
        case 'X':
            withBaseOption.useXdgPath = withoutBaseOption.useXdgPath = false;
            break;
        case 'h':
            usage();
            return 0;
        default:
            usage();
            return 1;
        }
    }

    if (optind + 1 != argc) {
        usage("Source file is missing.");
        return 1;
    }

    withBaseOption.sourceFile = withoutBaseOption.sourceFile = argv[optind];

    // Validate the required arguments.
    if (noBasefile) {
        return migrate(withoutBaseOption);
    }
    return migrate(withBaseOption);
}
