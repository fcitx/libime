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
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <fcitx-utils/charutils.h>
#include <fcitx-utils/standardpath.h>
#include <fcntl.h>
#if __GNUC__ < 8
#include <experimental/filesystem>
#else
#include <filesystem>
#endif
#include <istream>
#include <sstream>

#if defined(__linux__) || defined(__GLIBC__)
#include <endian.h>
#else
#include <sys/endian.h>
#endif

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
            StandardPath standardPath(/*skipFcitxPath=*/true);
            sourceFd = UnixFD::own(
                standardPath
                    .openUser(StandardPath::Type::Config,
                              stringutils::joinPath("fcitx/table", sourceFile),
                              O_RDONLY)
                    .release());
            if (!sourceFd.isValid()) {
                sourceFd = UnixFD::own(
                    standardPath
                        .open(StandardPath::Type::Config,
                              stringutils::joinPath("fcitx/table", sourceFile),
                              O_RDONLY)
                        .release());
            }
        } else {
            sourceFd = UnixFD::own(open(sourceFile.data(), O_RDONLY));
        }
        return sourceFd;
    }

    UnixFD openMergeFile(const std::string &path) const {
        UnixFD fd;
        if (useXdgPath && path[0] != '/') {
            fd = UnixFD::own(StandardPath::global()
                                 .openUser(StandardPath::Type::PkgData,
                                           stringutils::joinPath("table", path),
                                           O_RDONLY)
                                 .release());
        } else {
            fd = UnixFD::own(open(path.data(), O_RDONLY));
        }
        return fd;
    }

    std::string pathForSave(const std::string &path) const {
        if (path[0] != '/') {
            if (useXdgPath) {
                return stringutils::joinPath("table", path);
            } else {
#if __GNUC__ < 8
                return std::experimental::filesystem::absolute(path);
#else
                return std::filesystem::absolute(path);
#endif
            }
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
        << std::endl
        << "<source>: the source file of the dictionary." << std::endl
        << "-o: output dict file path" << std::endl
        << "-O: Skip dict file." << std::endl
        << "-p: history file path" << std::endl
        << "-P: Skip history file" << std::endl
        << "-b: base file of a libime main dict" << std::endl
        << "-B: generate full data without base file" << std::endl
        << "-X: locate non-abstract path by only path instead of Xdg path"
        << std::endl
        << "-U: overwrite instead of merge with existing data." << std::endl
        << "-h: Show this help" << std::endl;
    if (extra) {
        std::cout << extra << std::endl;
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
    unsigned char c;
    std::string_view punct = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    for (c = 0; c <= 127; c++) {
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
    std::function<void(const BasicTableInfo &info)> basicInfoCallback,
    std::function<void(const BasicTableInfo &info, RecordType,
                       const std::string &, const std::string &, uint32_t)>
        recordCallback) {
    BasicTableInfo info;
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source>
        buffer(sourceFd.fd(),
               boost::iostreams::file_descriptor_flags::never_close_handle);
    std::istream in(&buffer);

    uint32_t codeStrLength;
    throw_if_io_fail(unmarshallLE(in, codeStrLength));
    //先读取码表的信息
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
    std::string rule;
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
            ss << std::endl;
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

            switch (recordType) {
            case RECORDTYPE_PINYIN:
                break;
            case RECORDTYPE_CONSTRUCT:
                break;
            case RECORDTYPE_PROMPT:
                break;
            default:
                recordType = RECORDTYPE_NORMAL;
                break;
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

int migrate(MigrationWithBaseOption option) {
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
                baseFd = UnixFD::own(
                    StandardPath::global()
                        .open(fcitx::StandardPath::Type::PkgData,
                              stringutils::joinPath("table", *name), O_RDONLY)
                        .release());
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

    if (!option.skipDict) {
        if (option.dictFile.empty()) {
            if (!option.useXdgPath) {
                usage("Output dict file need to be specified.");
                return 1;
            }

            if (auto name =
                    replaceSuffix(option.sourceFile, mbSuffix, ".user.dict")) {
                option.dictFile = *name;
            } else {
                usage("Failed to infer the dict file name. Please use -o to "
                      "specifiy the dict file, or -O skip.");
                return 1;
            }
        }
    }

    if (!option.skipHistory) {
        if (option.historyFile.empty()) {
            if (!option.useXdgPath) {
                usage("History file need to be specified.");
                return 1;
            }

            if (auto name =
                    replaceSuffix(option.sourceFile, mbSuffix, ".history")) {
                option.historyFile = *name;
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
        boost::iostreams::stream_buffer<
            boost::iostreams::file_descriptor_source>
            buffer(baseFd.fd(),
                   boost::iostreams::file_descriptor_flags::never_close_handle);
        std::istream in(&buffer);
        tableDict.load(in);
    }
    if (option.merge && !option.skipDict) {
        UnixFD dictFd = option.openMergeFile(option.dictFile);
        if (dictFd.isValid()) {
            try {
                boost::iostreams::stream_buffer<
                    boost::iostreams::file_descriptor_source>
                    buffer(dictFd.fd(),
                           boost::iostreams::file_descriptor_flags::
                               never_close_handle);
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
        UnixFD historyFd = option.openMergeFile(option.historyFile);
        if (historyFd.isValid()) {
            try {
                boost::iostreams::stream_buffer<
                    boost::iostreams::file_descriptor_source>
                    buffer(historyFd.fd(),
                           boost::iostreams::file_descriptor_flags::
                               never_close_handle);
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
                         i < std::min(static_cast<uint32_t>(10u), freq); i++) {
                        history.add({value});
                    }
                }
            });
    } catch (const std::exception &e) {
        std::cout << "Failed when loading source file: " << e.what();
        return 1;
    }

    std::cout << "Found " << mergedWord << " new words." << std::endl;
    if (!option.skipDict) {
        if (!StandardPath::global().safeSave(
                StandardPath::Type::PkgData,
                option.pathForSave(option.dictFile), [&tableDict](int fd) {
                    boost::iostreams::stream_buffer<
                        boost::iostreams::file_descriptor_sink>
                        buffer(fd, boost::iostreams::file_descriptor_flags::
                                       never_close_handle);
                    std::ostream out(&buffer);
                    tableDict.saveUser(out);
                    return true;
                })) {
            std::cout << "Failed to save to dictionary file." << std::endl;
            return 1;
        }
    }

    if (!option.skipHistory) {
        if (!StandardPath::global().safeSave(
                StandardPath::Type::PkgData,
                option.pathForSave(option.historyFile), [&history](int fd) {
                    boost::iostreams::stream_buffer<
                        boost::iostreams::file_descriptor_sink>
                        buffer(fd, boost::iostreams::file_descriptor_flags::
                                       never_close_handle);
                    std::ostream out(&buffer);
                    history.save(out);
                    return true;
                })) {
            std::cout << "Failed to save to history file." << std::endl;
            return 1;
        }
    }

    return 0;
}

int migrate(MigrationWithoutBaseOption option) {

    if (!option.skipDict) {
        if (option.dictFile.empty()) {
            if (!option.useXdgPath) {
                usage("Output dict file need to be specified.");
                return 1;
            }

            if (auto name =
                    replaceSuffix(option.sourceFile, mbSuffix, ".user.dict")) {
                option.dictFile = *name;
            } else {
                usage("Failed to infer the dict file name. Please use -o to "
                      "specifiy the dict file, or -O skip.");
                return 1;
            }
        }
    }

    if (!option.skipHistory) {
        if (option.historyFile.empty()) {
            if (!option.useXdgPath) {
                usage("History file need to be specified.");
                return 1;
            }
            if (auto name =
                    replaceSuffix(option.sourceFile, mbSuffix, ".main.dict")) {
                option.historyFile = *name;
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
                ss << "KeyCode=" << info.code << std::endl;
                ss << "Length=" << info.length << std::endl;

                if (!info.ignoreChars.empty()) {
                    ss << "InvalidChar=" << info.ignoreChars << std::endl;
                }

                if (info.pinyin) {
                    ss << "Pinyin=" << info.pinyin << std::endl;
                }

                if (info.prompt) {
                    ss << "Prompt=" << info.prompt << std::endl;
                }

                if (info.phrase) {
                    ss << "ConstructPhrase=" << info.phrase << std::endl;
                }

                if (!info.rule.empty()) {
                    ss << "[Rule]\n" << info.rule << std::endl;
                }
                ss << "[Data]" << std::endl;
            },
            [&option, &ss, &history](const BasicTableInfo &info,
                                     RecordType type, const std::string &code,
                                     const std::string &value, uint32_t freq) {
                if (!option.skipDict) {
                    switch (type) {
                    case RECORDTYPE_NORMAL:
                        ss << code << " " << value << std::endl;
                        break;
                    case RECORDTYPE_CONSTRUCT:
                        if (info.phrase) {
                            ss << info.phrase << code << " " << value
                               << std::endl;
                        }
                        break;
                    case RECORDTYPE_PROMPT:
                        if (info.prompt) {
                            ss << info.prompt << code << " " << value
                               << std::endl;
                        }
                        break;
                    case RECORDTYPE_PINYIN:
                        if (info.pinyin) {
                            ss << info.pinyin << code << " " << value
                               << std::endl;
                        }
                        break;
                    }
                }

                if (!option.skipHistory) {
                    for (uint32_t i = 0;
                         i < std::min(static_cast<uint32_t>(10u), freq); i++) {
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
        if (!StandardPath::global().safeSave(
                StandardPath::Type::PkgData,
                option.pathForSave(option.dictFile), [&tableDict](int fd) {
                    boost::iostreams::stream_buffer<
                        boost::iostreams::file_descriptor_sink>
                        buffer(fd, boost::iostreams::file_descriptor_flags::
                                       never_close_handle);
                    std::ostream out(&buffer);
                    tableDict.save(out);
                    return true;
                })) {
            std::cout << "Failed to save to dictionary file." << std::endl;
            return 1;
        }
    }

    if (!option.skipHistory) {
        if (!StandardPath::global().safeSave(
                StandardPath::Type::PkgData,
                option.pathForSave(option.historyFile), [&history](int fd) {
                    boost::iostreams::stream_buffer<
                        boost::iostreams::file_descriptor_sink>
                        buffer(fd, boost::iostreams::file_descriptor_flags::
                                       never_close_handle);
                    std::ostream out(&buffer);
                    history.save(out);
                    return true;
                })) {
            std::cout << "Failed to save to history file." << std::endl;
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
            withBaseOption.historyFile = withoutBaseOption.historyFile = true;
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
    UnixFD baseFd, outputFd;

    // Validate the required arguments.
    if (noBasefile) {
        return migrate(withoutBaseOption);
    }
    return migrate(withBaseOption);
}
