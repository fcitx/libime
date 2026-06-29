# Pinyin dictionary text format

This document describes the plain-text format used by `PinyinDictionary`.

For in-repository examples, see `test/testpinyindictionary.cpp` and the
generated dictionary source used by tests such as `data/dict_sc.txt`.

## Overview

The format is line-based. Each dictionary entry is written on one line in one
of these forms:

```text
<hanzi> <full-pinyin>
<hanzi> <full-pinyin> <weight>
```

Examples:

```text
倪辉 ni'hui 0.0
小企鹅 xiao'qi'e
X光 X'guang
```

## Fields

### 1. Hanzi field

The first field is the word or phrase text.

Examples:

```text
你
小企鹅
X光
```

This field may be quoted when it contains whitespace, quotes, backslashes, or
escaped newlines.

Example:

```text
"你\n好 不\"" ni
```

That entry represents the text:

```text
你
好 不"
```

### 2. Full pinyin field

The second field is the full pinyin spelling.

Examples:

```text
ni
ni'hui
xiao'qi'e
X'guang
```

Important details:

- syllables are separated with `'`
- upper-case letters are allowed when the spelling needs them, for example
  `X'guang`
- the pinyin must be a valid full pinyin spelling accepted by libime

### 3. Weight field

The third field is optional and is parsed as a floating-point number.

Examples:

```text
0
0.0
1.25
-3
```

If the weight is omitted, the entry is loaded with `0.0`.

When the dictionary is saved back to text, every entry is written with an
explicit weight field, so an entry that was originally:

```text
小企鹅 xiao'qi'e
```

will be written back as:

```text
小企鹅 xiao'qi'e 0
```

## Escaping and quoting

Fields are tokenized with support for quoted and escaped values.

Use quotes when a field contains whitespace or special characters:

```text
"你\n好 不\"" ni
```

Supported escaped forms are the usual backslash-escaped forms accepted by the
value parser, including:

- `\"` for a literal quote
- `\\` for a literal backslash
- `\n` for an embedded newline

Each entry still occupies one physical line in the file. If you need embedded
newlines inside the text field, represent them with escapes such as `\n`.

## Line behavior

- Blank lines are ignored.
- Lines with anything other than 2 or 3 parsed fields are ignored.
- There is no section syntax in this format.
- Comment syntax is not part of the format. A line beginning with `#` is simply
  treated as an invalid entry and ignored unless it happens to parse as a valid
  2-field or 3-field record.

## Validation behavior

An entry is skipped if:

- the pinyin field is invalid
- the optional weight field is present but is not a valid floating-point number

The loader continues reading later lines after such errors.

## Saved text format

When written back as text, each entry is saved in this form:

```text
<escaped-hanzi> <full-pinyin> <weight>
```

Examples:

```text
X光 X'guang 0
"你\\n好 不\\\"" ni 0
倪辉 ni'hui 0
```

The hanzi field is automatically escaped when needed.
