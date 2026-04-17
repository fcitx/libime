/*
 * SPDX-FileCopyrightText: 2011-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef SPDATA_H
#define SPDATA_H

struct SP_C {
    char strQP[5];
    char cJP;
};

struct SP_S {
    char strQP[3];
    char cJP;
};

static const SP_C SPMap_C_MS[] = {
    {"ai", 'l'},   {"an", 'j'},  {"ang", 'h'},  {"ao", 'k'}, {"ei", 'z'},
    {"en", 'f'},   {"eng", 'g'}, {"er", 'r'},   {"ia", 'w'}, {"ian", 'm'},
    {"iang", 'd'}, {"iao", 'c'}, {"ie", 'x'},   {"in", 'n'}, {"ing", ';'},
    {"iong", 's'}, {"iu", 'q'},  {"ong", 's'},  {"ou", 'b'}, {"ua", 'w'},
    {"uai", 'y'},  {"uan", 'r'}, {"uang", 'd'}, {"ue", 't'}, {"ui", 'v'},
    {"un", 'p'},   {"uo", 'o'},  {"ve", 'v'},   {"v", 'y'},  {"\0", '\0'}};

static const SP_S SPMap_S_MS[] = {
    {"ch", 'i'}, {"sh", 'u'}, {"zh", 'v'}, {"\0", '\0'}};

static const SP_C SPMap_C_Ziguang[] = {
    {"ai", 'p'},   {"an", 'r'},  {"ang", 's'},  {"ao", 'q'}, {"ei", 'k'},
    {"en", 'w'},   {"eng", 't'}, {"er", 'j'},   {"ia", 'x'}, {"ian", 'f'},
    {"iang", 'g'}, {"iao", 'b'}, {"ie", 'd'},   {"in", 'y'}, {"ing", ';'},
    {"iong", 'h'}, {"iu", 'j'},  {"ong", 'h'},  {"ou", 'z'}, {"ua", 'x'},
    {"uai", 'y'},  {"uan", 'l'}, {"uang", 'g'}, {"ue", 'n'}, {"ui", 'n'},
    {"un", 'm'},   {"uo", 'o'},  {"ve", 'n'},   {"v", 'v'},  {"\0", '\0'}};

static const SP_S SPMap_S_Ziguang[] = {
    {"ch", 'a'}, {"sh", 'i'}, {"zh", 'u'}, {"\0", '\0'}};

static const SP_C SPMap_C_ABC[] = {
    {"ai", 'l'},   {"an", 'j'},  {"ang", 'h'},  {"ao", 'k'}, {"ei", 'q'},
    {"en", 'f'},   {"eng", 'g'}, {"er", 'r'},   {"ia", 'd'}, {"ian", 'w'},
    {"iang", 't'}, {"iao", 'z'}, {"ie", 'x'},   {"in", 'c'}, {"ing", 'y'},
    {"iong", 's'}, {"iu", 'r'},  {"ong", 's'},  {"ou", 'b'}, {"ua", 'd'},
    {"uai", 'c'},  {"uan", 'p'}, {"uang", 't'}, {"ue", 'm'}, {"ui", 'm'},
    {"un", 'n'},   {"uo", 'o'},  {"ve", 'm'},   {"v", 'v'},  {"\0", '\0'}};

static const SP_S SPMap_S_ABC[] = {
    {"ch", 'e'}, {"sh", 'v'}, {"zh", 'a'}, {"\0", '\0'}};

static const SP_C SPMap_C_Zhongwenzhixing[] = {
    {"ai", 's'},   {"an", 'f'},  {"ang", 'g'},  {"ao", 'd'}, {"ei", 'w'},
    {"en", 'r'},   {"eng", 't'}, {"er", 'q'},   {"ia", 'b'}, {"ian", 'j'},
    {"iang", 'h'}, {"iao", 'k'}, {"ie", 'm'},   {"in", 'l'}, {"ing", 'q'},
    {"iong", 'y'}, {"iu", 'n'},  {"ong", 'y'},  {"ou", 'p'}, {"ua", 'b'},
    {"uai", 'x'},  {"uan", 'c'}, {"uang", 'h'}, {"ue", 'x'}, {"ui", 'v'},
    {"un", 'z'},   {"uo", 'o'},  {"ve", 'x'},   {"v", 'v'},  {"\0", '\0'}};

static const SP_S SPMap_S_Zhongwenzhixing[] = {
    {"ch", 'u'}, {"sh", 'i'}, {"zh", 'v'}, {"\0", '\0'}};

static const SP_C SPMap_C_PinyinJiaJia[] = {
    {"ai", 's'},   {"an", 'f'},  {"ang", 'g'},  {"ao", 'd'}, {"ei", 'w'},
    {"en", 'r'},   {"eng", 't'}, {"er", 'q'},   {"ia", 'b'}, {"ian", 'j'},
    {"iang", 'h'}, {"iao", 'k'}, {"ie", 'm'},   {"in", 'l'}, {"ing", 'q'},
    {"iong", 'y'}, {"iu", 'n'},  {"ong", 'y'},  {"ou", 'p'}, {"ua", 'b'},
    {"uai", 'x'},  {"uan", 'c'}, {"uang", 'h'}, {"ue", 'x'}, {"ui", 'v'},
    {"un", 'z'},   {"uo", 'o'},  {"ve", 'x'},   {"v", 'v'},  {"\0", '\0'}};

static const SP_S SPMap_S_PinyinJiaJia[] = {
    {"ch", 'u'}, {"sh", 'i'}, {"zh", 'v'}, {"\0", '\0'}};

static const SP_C SPMap_C_Ziranma[] = {
    {"ai", 'l'},   {"an", 'j'},  {"ang", 'h'},  {"ao", 'k'}, {"ei", 'z'},
    {"en", 'f'},   {"eng", 'g'}, {"er", 'r'},   {"ia", 'w'}, {"ian", 'm'},
    {"iang", 'd'}, {"iao", 'c'}, {"ie", 'x'},   {"in", 'n'}, {"ing", 'y'},
    {"iong", 's'}, {"iu", 'q'},  {"ong", 's'},  {"ou", 'b'}, {"ua", 'w'},
    {"uai", 'y'},  {"uan", 'r'}, {"uang", 'd'}, {"ue", 't'}, {"ui", 'v'},
    {"un", 'p'},   {"uo", 'o'},  {"ve", 't'},   {"v", 'v'},  {"\0", '\0'}};

static const SP_S SPMap_S_Ziranma[] = {
    {"ch", 'i'}, {"sh", 'u'}, {"zh", 'v'}, {"\0", '\0'}};

static const SP_C SPMap_C_XIAOHE[] = {
    {"ai", 'd'},  {"an", 'j'},   {"ang", 'h'}, {"ao", 'c'},  {"ei", 'w'},
    {"en", 'f'},  {"eng", 'g'},  {"ia", 'x'},  {"ian", 'm'}, {"iang", 'l'},
    {"iao", 'n'}, {"ie", 'p'},   {"in", 'b'},  {"ing", 'k'}, {"iong", 's'},
    {"iu", 'q'},  {"ong", 's'},  {"ou", 'z'},  {"ua", 'x'},  {"uai", 'k'},
    {"uan", 'r'}, {"uang", 'l'}, {"ue", 't'},  {"ui", 'v'},  {"un", 'y'},
    {"uo", 'o'},  {"ve", 't'},   {"v", 'v'},   {"\0", '\0'}};

static const SP_S SPMap_S_XIAOHE[] = {
    {"ch", 'i'}, {"sh", 'u'}, {"zh", 'v'}, {"\0", '\0'}};

static const SP_C SPMap_C_GB[] = {
    {.strQP = "ai", .cJP = 'k'},   {.strQP = "an", .cJP = 'f'},
    {.strQP = "ang", .cJP = 'g'},  {.strQP = "ao", .cJP = 'c'},
    {.strQP = "ei", .cJP = 'b'},   {.strQP = "en", .cJP = 'r'},
    {.strQP = "eng", .cJP = 'h'},  {.strQP = "er", .cJP = 'l'},
    {.strQP = "ia", .cJP = 'q'},   {.strQP = "ian", .cJP = 'd'},
    {.strQP = "iang", .cJP = 'n'}, {.strQP = "iao", .cJP = 'm'},
    {.strQP = "ie", .cJP = 't'},   {.strQP = "in", .cJP = 'l'},
    {.strQP = "ing", .cJP = 'j'},  {.strQP = "iong", .cJP = 's'},
    {.strQP = "iu", .cJP = 'y'},   {.strQP = "ong", .cJP = 's'},
    {.strQP = "ou", .cJP = 'p'},   {.strQP = "ua", .cJP = 'q'},
    {.strQP = "uai", .cJP = 'y'},  {.strQP = "uan", .cJP = 'w'},
    {.strQP = "uang", .cJP = 'n'}, {.strQP = "ue", .cJP = 'x'},
    {.strQP = "ui", .cJP = 'v'},   {.strQP = "un", .cJP = 'z'},
    {.strQP = "uo", .cJP = 'o'},   {.strQP = "ve", .cJP = 'x'},
    {.strQP = "v", .cJP = 'v'},    {.strQP = "\0", .cJP = '\0'}};

static const SP_S SPMap_S_GB[] = {{.strQP = "ch", .cJP = 'i'},
                                  {.strQP = "sh", .cJP = 'u'},
                                  {.strQP = "zh", .cJP = 'v'},
                                  {.strQP = "\0", .cJP = '\0'}};

#endif
