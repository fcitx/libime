# Shuangpin profile text format

This document describes the text format for a custom shuangpin profile such
as `sp.dat`.

For an in-repository example, see `test/testshuangpinprofile.cpp`.

## Overview

A shuangpin profile is a UTF-8 text file. In practice it is usually written
with these sections:

1. `[方案]`
2. `[零声母标识]`
3. `[声母]`
4. `[韵母]`
5. `[音节]`

Example:

```text
[方案]
方案名称=自定义

[零声母标识]
=O*

[声母]
ch=I
sh=U
zh=V

[韵母]
ai=L
an=J
ang=H
ao=K
ei=Z
en=F
eng=G
er=R
ia=W
ian=M
iang=D
iao=C
ie=X
in=N
ing=Y
iong=S
iu=Q
ong=S
ou=B
ua=W
uai=Y
uan=R
uang=D
ue=T
ui=V
un=P
ve=T
uo=O

[音节]
ju=jv
shi=ui
e=ee
```

## Line rules

- The file is line-based.
- Blank lines are ignored.
- Lines whose first non-space character is `#` are comments.
- Leading and trailing whitespace is ignored.
- Keys on the right-hand side are case-insensitive; they are normalized to
  lowercase.

Section headers such as `[方案]` and `[韵母]` are recommended for readability,
but the meaningful content is carried by the mapping lines themselves.

## 1. Scheme name

Use:

```text
方案名称=<name>
```

Example:

```text
方案名称=自定义
```

For a custom profile, use a custom name such as `自定义` or any other non-built-in
name.

These built-in names are special:

- `自然码`
- `微软`
- `紫光`
- `拼音加加`
- `中文之星`
- `智能ABC`
- `小鹤`

If `方案名称=` is set to one of those names, the remaining custom mapping lines in
the file are ignored. For a custom profile, avoid using those names.

## 2. Zero-initial marker

Use:

```text
=<chars>
```

Examples:

```text
=O*
=a
=o
```

Meaning:

- every non-`*` character in `<chars>` can be used as the first key of a
  zero-initial syllable
- the value is case-insensitive

Examples:

- `=o` allows `o` as a zero-initial marker
- `=a` allows `a` as a zero-initial marker
- `=O*` means `o` is a zero-initial marker and `*` enables the special
  Ziranma/Xiaohe-style zero-initial rules described below

### `*` special behavior

If the marker string contains `*`, zero-initial syllables are additionally
generated in this style:

1. one-letter finals use a doubled key, such as `aa`, `ee`
2. two-letter finals keep their quanpin form, such as `ou`, `en`
3. longer finals use the first letter of full pinyin plus the mapped final key

For example, if `ang=g`, then zero-initial `ang` can be entered as:

```text
ag
```

### Default behavior

If no valid zero-initial marker line is provided, the default marker is `o`.

A line containing only:

```text
=
```

does not define an empty marker; it leaves the default behavior unchanged.

## 3. Initial mappings

Use:

```text
<initial>=<key>
```

Examples:

```text
ch=u
sh=i
zh=v
```

The right-hand side must be exactly one character.

Multiple mappings are allowed:

```text
ch=u
ch=;
zh=o
zh=v
```

You do not need to list initials whose shuangpin key is already the same as
their normal one-letter initial. For example, `b=b` and `m=m` are unnecessary.

## 4. Final mappings

Use:

```text
<final>=<key>
```

Examples:

```text
iu=q
ian=w
uang=z
ui=;
v=v
```

The right-hand side must be exactly one character.

Multiple mappings are allowed:

```text
ua=a
ua=m
uo=p
uo=b
```

Single-letter finals such as `a`, `e`, `i`, `o`, `u`, `v` do not always need to
be listed. They already work as direct inputs unless you want to override or
add aliases.

## 5. Explicit syllable mappings

Use:

```text
<full-pinyin>=<two-key-input>
```

Examples:

```text
ju=jv
e=ee
e=e;
shi=is
zhong=os
yue=yb
```

This is used for explicit full-syllable overrides or aliases that cannot be
described well enough by only separate initial/final mappings.

Rules:

- the left-hand side must be a valid full pinyin syllable
- the right-hand side must be exactly two characters
- the two-key input is case-insensitive
- multiple explicit mappings are allowed for the same syllable

This section is especially useful for:

- zero-initial syllables such as `e`, `ou`, `er`
- syllables with multiple accepted shuangpin forms
- compatibility aliases
- custom disambiguation choices

## How the sections work together

The profile is built from three kinds of information:

1. initial mappings
2. final mappings
3. explicit full-syllable mappings

In addition:

- one-letter initials are accepted directly even if they are not listed
- one-letter finals are also available directly
- explicit syllable mappings add extra accepted two-key forms
- multiple mappings for the same initial, final, or syllable are allowed

## Supported characters on the right-hand side

The key characters are not limited to letters. Punctuation such as `;` is also
allowed, as shown in `test/testshuangpinprofile.cpp`.

Examples:

```text
ch=;
ue=;
ui=;
```

Each mapping must stay on a single line.
