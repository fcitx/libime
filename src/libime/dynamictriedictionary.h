#ifndef LIBIME_DYNAMICTRIEDICTIONARY_H
#define LIBIME_DYNAMICTRIEDICTIONARY_H

#include <istream>
#include <ostream>
#include <memory>

#include "dictionary.h"

namespace libime {

class StringEncoder;

    class DynamicTrieDictionary : Dictionary
    {
    public:
        DynamicTrieDictionary(char delim=' ', char linedelim='\n');

        virtual void lookup(const std::string& input, std::vector<std::string>& output) override;
        void read(std::istream& in);
        void write(std::ostream& out);
        void setStringEncoder(const std::shared_ptr<StringEncoder>& encoder);
        void insert(const std::string& input, const std::string& output);
        void remove(const std::string& input, const std::string& output);

    private:
        class Private;
        std::unique_ptr<Private> d;
    };

}

#endif // LIBIME_DYNAMICTRIEDICTIONARY_H
