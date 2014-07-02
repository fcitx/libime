#include <iostream>
#include <assert.h>
#include <unicode/ustream.h>
#include "libime/rawinput.h"
#include "libime/splitter.h"
#include "libime/preeditformatter.h"

using namespace libime;

int main()
{
    std::shared_ptr<RawInput> rawInput(new RawInput);
    std::shared_ptr<Pipeline> splitter(new Splitter(" "));
    std::shared_ptr<SimplePreeditFormatter> formatter(new SimplePreeditFormatter('_'));

    rawInput->addSink(std::dynamic_pointer_cast<Sink>(splitter));
    splitter->addSink(std::dynamic_pointer_cast<Sink>(formatter));

    rawInput->append('a');
    rawInput->append('p');
    rawInput->append('p');
    rawInput->append('l');
    rawInput->append('e');
    rawInput->append(' ');
    rawInput->append('r');
    rawInput->append('e');
    rawInput->append('d');

    assert(formatter->preedit() == UNICODE_STRING_SIMPLE("apple_red"));
    
    return 0;
}
