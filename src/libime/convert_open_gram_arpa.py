#!/usr/bin/python3
import sys
import fileinput

def is_prob(s):
    return len(s) > 2 and '.' in s and s[0].isdigit()


lines=list(fileinput.input())
print("\\data\\")
count=dict()
for line in lines:
    line=line.strip()
    if "gram" in line:
        num = line[1:line.index("-gram")]
        if num == "0":
            continue
        tokens = line.split('\\')
        count[num] = tokens[-1]

for c in sorted(count):
    print("ngram {0}={1}".format(c, count[c]))

# empty line
print()

in_gram=False
for line in lines:
    line=line.strip()
    if "gram" in line:
        num = line[1:line.index("-gram")]
        if num != "0":
            in_gram=True
            print("\\{0}-grams:".format(num))
    elif in_gram:
        tokens = [t if t != "<unknown>" else "<unk>" for t in line.split()[:-1]]
        if len(tokens) < 3:
            continue
        if is_prob(tokens[-1]):
            if is_prob(tokens[-2]):
                tokens=["-" + tokens[-2]] + tokens[0:-2] + ["-" + tokens[-1]]
            else:
                tokens=["-" + tokens[-1]] + tokens[0:-1]
            print("\t".join(tokens))
        else:
            print(tokens, file=sys.stderr)
print("\\end\\")
