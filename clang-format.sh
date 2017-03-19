#!/bin/sh
find . \( -name '*.h' -o -name '*.cpp' \) -not -path "./src/libime/kenlm/*" -not -path "./build/*"  | xargs clang-format -i
