#!/bin/sh
find . \( -name '*.h' -o -name '*.cpp' \) -not -path "./src/libime/core/kenlm/*" -not -path "./build/*"  | xargs clang-format -i
