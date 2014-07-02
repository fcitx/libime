#include <iostream>
#include "libime/slm/languagemodel.h"

int main(int argc, char* argv[]) {
    using namespace libime;
    LanguageModel model(argv[1]);
    State state(model.beginSentenceState()), out_state;
    std::string word;
    while (std::cin >> word) {
        std::cout << model.score(state, word, out_state) << '\n';
        state = out_state;
    }

    return 0;
}
