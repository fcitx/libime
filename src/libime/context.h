#ifndef LIBIME_LANGUAGEMODELCONTEXT_H
#define LIBIME_LANGUAGEMODELCONTEXT_H

namespace libime
{
    class LanguageModelContext
    {
    public:
        virtual ~LanguageModelContext();

        virtual double score();
        virtual reset() = 0;
    };
}

#endif // LIBIME_LANGUAGEMODELCONTEXT_H
