#ifndef LIBIME_LANGUAGEMODEL_H
#define LIBIME_LANGUAGEMODEL_H

#include <memory>
#include <vector>
#include <algorithm>
#include "common.h"

/**
 * kenlm recommends other to directly include their source, while
 * it gives no promise about the API stability.
 *
 * slm library is going to create a thin wrapper over kenlm for
 * common slm usecase for input method, with stable APIs.
 */

namespace libime
{
    struct Entry
    {
        std::string input;
        std::string output;
        unsigned int id;
    };

    class LIBIME_API LanguageModel
    {
    public:
        virtual ~LanguageModel();

        std::vector<Entry> entries(const std::vector<std::string>& input);
    };
}

#endif // LIBIME_LANGUAGEMODEL_H
