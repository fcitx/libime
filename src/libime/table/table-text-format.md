# Table file format

A table file is a UTF-8 text file with up to four parts, in this order:

1. config
2. optional rule section
3. data section
4. optional phrase section

Both English and Chinese keywords are accepted:

| English | Chinese |
| --- | --- |
| `KeyCode=` | `键码=` |
| `Length=` | `码长=` |
| `InvalidChar=` | `规避字符=` |
| `Pinyin=` | `拼音=` |
| `PinyinLength=` | `拼音长度=` |
| `Prompt=` | `提示=` |
| `ConstructPhrase=` | `构词=` |
| `[Rule]` | `[组词规则]` |
| `[Data]` | `[数据]` |
| `[Phrase]` | `[词组]` |

## Example

```text
KeyCode=abcdefghijklmnopqrstuvwxy
Length=4
Pinyin=@
[Rule]
e2=p11+p12+p21+p22
e3=p11+p21+p31+p32
a4=p11+p21+p31+n11
[Data]
xycq 统
yfh 计
nnkd 局
[Phrase]
统计局
```

## Typical use cases

### Chinese table input methods

For Chinese table input methods such as Wubi, Cangjie, Zhengma, or other
shape-based systems, a table file often needs more than simple `code -> text`
entries.

Common needs:

- single-character entries in `[Data]`
- phrase generation rules in `[Rule]`
- optional `[Phrase]` entries for common multi-character words
- optional `Pinyin=` entries for pinyin-assisted lookup
- optional `Prompt=` entries for decomposition or radical hints
- optional `ConstructPhrase=` entries when phrase generation should use a
  dedicated per-character construction code

This is the typical full-featured table format.

### Other languages or simpler code tables

For non-Chinese input methods, symbol tables, transliteration schemes, or any
other simple code-based lookup table, usually only the basic configuration and
`[Data]` section are needed.

Typical cases:

- custom symbol input
- shorthand expansion
- transliteration-based word lookup
- small code-to-text dictionaries

In these cases, you can often omit:

- `[Rule]`
- `[Phrase]`
- `Pinyin=`
- `Prompt=`
- `ConstructPhrase=`

The minimal practical form is:

```text
KeyCode=abcdefghijklmnopqrstuvwxyz
Length=6
[Data]
ab example
abc another
```

## 1. Configuration

The file starts with configuration lines. Leading and trailing whitespace is
ignored. Lines whose first non-space character is `#` are comments. This comment
rule applies only to the configuration and rule sections, not to the `[Data]`
and `[Phrase]` sections.

Supported keys:

- `KeyCode=`: required set of valid input-code characters.
- `Length=`: maximum code length for normal entries.
- `InvalidChar=`: code characters that should not be used as the first
  character of a single-character entry when building the per-character lookup
  used for phrase generation. This is mainly useful for special leading codes in
  some tables.
- `Pinyin=`: one character used as the prefix for pinyin entries in `[Data]`.
- `Prompt=`: one character used as the prefix for prompt/hint entries in
  `[Data]`.
- `ConstructPhrase=`: one character used as the prefix for phrase-construction
  entries in `[Data]`.
- `PinyinLength=`: reserved for compatibility.

`[Data]` ends the config section and starts the main dictionary data.

`[Rule]` switches to the rule section. A later `[Data]` is still required.

## 2. Rule section

The rule section is optional. It is only needed when you want phrase codes to
be generated automatically.

For Wubi, `wbx.txt` uses this rule set:

```text
[组词规则]
e2=p11+p12+p21+p22
e3=p11+p21+p31+p32
a4=p11+p21+p31+n11
```

Rule syntax:

- `eN=...`: apply when phrase length is exactly `N`.
- `aN=...`: apply when phrase length is at least `N`.
- The right-hand side is a `+`-joined list of rule entries.
- Each rule entry is three characters:
  - `p` or `n`: pick the phrase character from the front or the back.
  - the second character: which phrase character to use (`1`-`9`).
  - the third character: which code position to use from that character's
    code:
    - `1`-`9`: from the front
    - `z`: last code element
    - `y`: second last
    - `x`: third last
    - and so on
- `p00` is a placeholder and contributes nothing.

For example, `p1z` means "take the last code element of the first character in
the phrase".

### Wubi rule meaning

These three rules are the standard phrase-building rules used by `wbx.txt`:

- `e2=p11+p12+p21+p22`
  - 2-character phrase
  - take the first two code elements from the first character
  - then the first two code elements from the second character
- `e3=p11+p21+p31+p32`
  - 3-character phrase
  - take the first code element from the first, second, and third characters
  - then take the second code element from the third character
- `a4=p11+p21+p31+n11`
  - phrase length 4 or more
  - take the first code element from the first three characters
  - then take the first code element from the last character

### Wubi example: `百合 -> djwg`

Assume the table already contains these single-character codes from `wbx.txt`:

```text
dj 百
wgk 合
```

For a 2-character phrase, Wubi uses:

```text
e2=p11+p12+p21+p22
```

This rule means:

- `e2`: apply to a 2-character phrase
- `p11`: take the 1st code element from the 1st character
- `p12`: take the 2nd code element from the 1st character
- `p21`: take the 1st code element from the 2nd character
- `p22`: take the 2nd code element from the 2nd character

Applying it to `百合`:

- `百` has code `dj`, so it contributes `d` and `j`
- `合` has code `wgk`, so it contributes `w` and `g`

The generated phrase code is therefore:

```text
百合 -> djwg
```

`[Data]` ends the rule section.

## 3. Data section

Each data line contains a key and a value, separated by whitespace:

```text
<key> <value>
```

Examples:

```text
a 日
aa 昌
abac 暝
```

Normal entries use only characters listed by `KeyCode=` in `<key>`.

If `Pinyin=`, `Prompt=`, or `ConstructPhrase=` is configured, a key may start
with that prefix character:

```text
@ni 你
&a 日
^cq 统
```

These mean:

- `@ni 你`: pinyin entry for `你`
- `&a 日`: prompt/hint text for code `a`
- `^cq 统`: construction code for phrase generation

Important details:

- The key must not be empty.
- `Prompt=` entries provide hint text, not normal dictionary candidates.
- `ConstructPhrase=` entries are used only for phrase-code generation. They are
  not inserted as normal candidates by themselves.
- Blank or malformed data lines are ignored.
- In `[Data]`, `#` is **not** a comment marker; it is just text unless it makes
  the line malformed.

## 4. Phrase section

`[Phrase]` is optional and may appear only after `[Data]`. It is valid only
when rules are present.

Each following line is a phrase value only:

```text
[Phrase]
统计局
中华人民共和国
```

The phrase code is generated from `[Rule]` plus the single-character entries
already defined earlier in the file.

After `[Phrase]`, every remaining line is treated as a phrase.

## Escaping

Values in `[Data]` and lines in `[Phrase]` may be quoted. Use quoting when a
value contains whitespace, `"` or `\`.

Each record in the file is still line-based: one entry per physical line.
Literal multi-line entries are not written as raw multi-line text inside the
file.

If you need a value to contain an embedded newline, write it with escaping
inside a quoted string, for example `\n`.

Examples:

```text
aaaa "工 "
f "\""
note "first line\nsecond line"
[Phrase]
"统计局"
```

Quoted values may use backslash escaping, including:

- `\"` for a literal quote
- `\\` for a literal backslash
- `\n` for an embedded newline

So this:

```text
note "first line\nsecond line"
```

represents a single value containing two lines, even though the table file
itself still stores it as one physical line.

## Requirements and restrictions

- The file must be valid UTF-8.
- `KeyCode=` must not contain the configured `Pinyin=`, `Prompt=`, or
  `ConstructPhrase=` prefix character.
- Data-entry keys must not be empty.
- `[Data]` is required.
- `[Phrase]` requires a rule section.
