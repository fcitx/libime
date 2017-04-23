#!/usr/bin/python3
import sys
import fileinput
import math


lines=list(fileinput.input())
for line in lines:
    line = line.strip()
    tokens = line.split()
    if len(tokens) >= 3:
        hanzi = tokens[0]
        for token in tokens[2:]:
            colon = token.find(':')
            if colon < 0:
                prob = math.log10(1.0 / (len(tokens) - 2))
                pinyin = token
            else:
                prob = math.log10(float(token[colon+1:-1]) / 100.0)
                pinyin = token[0:colon]
            print("\t".join([hanzi, pinyin, str(prob)]))
