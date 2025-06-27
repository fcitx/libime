/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <fcitx-utils/log.h>
#include "libime/core/segmentgraph.h"
#include "libime/pinyin/pinyindata.h"
#include "libime/pinyin/pinyinencoder.h"
#include "libime/pinyin/shuangpinprofile.h"
using namespace libime;

void checkProfile(const ShuangpinProfile &profile, bool hasSemicolon) {

    for (const auto &p : profile.table()) {
        if (!p.second.empty()) {
            std::cout << p.first;
            for (const auto &py : p.second) {
                std::cout << " " << py.first.toString() << " "
                          << static_cast<int>(py.second);
            }
            std::cout << std::endl;
        }
    }

    std::set<PinyinSyllable> validSyls;
    for (const auto &p : getPinyinMap()) {
        validSyls.emplace(p.initial(), p.final());
    }
    validSyls.erase(PinyinSyllable(PinyinInitial::M, PinyinFinal::Zero));
    validSyls.erase(PinyinSyllable(PinyinInitial::N, PinyinFinal::Zero));
    validSyls.erase(PinyinSyllable(PinyinInitial::R, PinyinFinal::Zero));
    validSyls.erase(PinyinSyllable(PinyinInitial::Zero, PinyinFinal::NG));

    for (const auto &p : profile.table()) {
        for (const auto &py : p.second) {
            // check coverage
            if (py.second == PinyinFuzzyFlag::None) {
                validSyls.erase(py.first);
            }
        }
    }

    std::set<char> initials = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
                               'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                               's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
    if (hasSemicolon) {
        initials.insert(';');
    }

    for (auto c : profile.validInitial()) {
        FCITX_ASSERT(initials.count(c))
            << " " << c << " " << profile.validInitial();
    }

    FCITX_ASSERT(validSyls.empty()) << validSyls;
}

void checkABC() {
    ShuangpinProfile sp(ShuangpinBuiltinProfile::ABC);
    auto iter = sp.table().find("an");
    FCITX_ASSERT(iter != sp.table().end());
    FCITX_ASSERT(iter->second.count(
        PinyinSyllable(PinyinInitial::ZH, libime::PinyinFinal::UN)));
    FCITX_ASSERT(!iter->second.count(
        PinyinSyllable(PinyinInitial::Zero, libime::PinyinFinal::AN)));
}

void checkXiaoHe() {
    ShuangpinProfile profile(ShuangpinBuiltinProfile::Xiaohe);
    FCITX_ASSERT(profile.table().count("lv") == 1);
    FCITX_ASSERT(profile.table().find("lv")->second.count(PinyinSyllable(
                     PinyinInitial::L, libime::PinyinFinal::IU)) == 0);
    PinyinEncoder::parseUserShuangpin("aiaaah", profile, PinyinFuzzyFlag::None)
        .dfs([](const SegmentGraphBase &segs, const std::vector<size_t> &path) {
            size_t s = 0;
            for (auto e : path) {
                std::cout << segs.segment(s, e) << " ";
                s = e;
            }
            std::cout << std::endl;
            return true;
        });
}

void checkZiranma() {
    ShuangpinProfile profile(ShuangpinBuiltinProfile::Ziranma);
    FCITX_ASSERT(profile.table().count("ng") == 1);
    FCITX_ASSERT(profile.table().find("ng")->second.count(PinyinSyllable(
                     PinyinInitial::N, libime::PinyinFinal::ENG)) == 1);
    FCITX_ASSERT(profile.table().find("ng")->second.count(PinyinSyllable(
                     PinyinInitial::Zero, libime::PinyinFinal::NG)) == 0);
    PinyinEncoder::parseUserShuangpin("aiaaah", profile, PinyinFuzzyFlag::None)
        .dfs([](const SegmentGraphBase &segs, const std::vector<size_t> &path) {
            size_t s = 0;
            for (auto e : path) {
                std::cout << segs.segment(s, e) << " ";
                s = e;
            }
            std::cout << std::endl;
            return true;
        });
}

void checkSegments(const libime::SegmentGraph &graph,
                   std::vector<std::vector<std::string>> segments) {
    graph.dfs([&segments](const SegmentGraphBase &segs,
                          const std::vector<size_t> &path) {
        size_t s = 0;
        std::vector<std::string> segment;
        for (auto e : path) {
            segment.push_back(std::string(segs.segment(s, e)));
            std::cout << segment.back() << " ";
            s = e;
        }
        segments.erase(std::remove(segments.begin(), segments.end(), segment),
                       segments.end());
        std::cout << std::endl;
        return true;
    });
    FCITX_ASSERT(segments.empty()) << "Remaining segments: " << segments;
}

void checkSimpleParsing() {
    struct {
        const char *qp;
        const char *sp;
    } zrmZero[] = {
        {"a", "aa"},   {"ai", "ai"}, {"an", "an"}, {"ang", "ah"},
        {"ao", "ao"},  {"e", "ee"},  {"ei", "ei"}, {"en", "en"},
        {"eng", "eg"}, {"er", "er"}, {"o", "oo"},  {"ou", "ou"},
    };
    ShuangpinProfile zrm(ShuangpinBuiltinProfile::Ziranma);
    for (auto [qp, sp] : zrmZero) {
        auto matchedSp =
            PinyinEncoder::shuangpinToSyllables(sp, zrm, PinyinFuzzyFlag::None);
        auto matchedQp =
            PinyinEncoder::stringToSyllables(qp, PinyinFuzzyFlag::None);
        FCITX_ASSERT(matchedQp == matchedSp)
            << " " << matchedQp << " " << matchedSp;
    }

    {
        auto partialSp = PinyinEncoder::shuangpinToSyllables(
            "z", zrm, PinyinFuzzyFlag::None);
        MatchedPinyinSyllables expected{
            {PinyinInitial::Z, {{PinyinFinal::Invalid, false}}}};
        FCITX_ASSERT(partialSp == expected) << partialSp;
    }

    std::string zrmText = "[方案]\n"
                          "方案名称=自定义\n"
                          "\n"
                          "[零声母标识]\n"
                          "=O*\n"
                          "\n"
                          "[声母]\n"
                          "# 双拼编码就是它本身的声母不必列出\n"
                          "ch=I\n"
                          "sh=U\n"
                          "zh=V\n"
                          "\n"
                          "[韵母]\n"
                          "# 双拼编码就是它本身的韵母不必列出\n"
                          "ai=L\n"
                          "an=J\n"
                          "ang=H\n"
                          "ao=K\n"
                          "ei=Z\n"
                          "en=F\n"
                          "eng=G\n"
                          "er=R\n"
                          "ia=W\n"
                          "ian=M\n"
                          "iang=D\n"
                          "iao=C\n"
                          "ie=X\n"
                          "in=N\n"
                          "ing=Y\n"
                          "iong=S\n"
                          "iu=Q\n"
                          "ong=S\n"
                          "ou=B\n"
                          "ua=W\n"
                          "uai=Y\n"
                          "uan=R\n"
                          "uang=D\n"
                          "ue=T\n"
                          "ui=V\n"
                          "un=P\n"
                          "ve=T\n"
                          "uo=O\n";
    std::stringstream ss(zrmText);
    ShuangpinProfile profile(ss);
    FCITX_ASSERT(profile.table() == zrm.table());
    FCITX_ASSERT(profile.validInput() == zrm.validInput());

    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("bu", zrm) == "bu");
    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("ds", zrm) == "dong");
    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("dm", zrm) == "dian");
    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("zu", zrm) == "zu");
    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("he", zrm) == "he");
    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("zi", zrm) == "zi");
}

void checkAdvanceParsing() {
    std::string csp = R"RAW_TEXT(
[方案]
方案名称=自定义

[零声母标识]
=

[声母]
# 双拼编码就是它本身的声母不必列出
ch=u
ch=;
sh=i
zh=o
zh=v

[韵母]
# 双拼编码就是它本身的韵母不必列出
iu=q
ian=w
uan=r
in=t
ua=a
ua=m
ie=y
ou=o
uo=p
uo=b
a=a
ia=l
ong=l
ei=s
eng=d
ing=f
ang=g
iang=g
ai=h
ao=j
iao=j
en=k
iong=l
ue=;
ve=;
uang=z
uai=x
un=c
ui=;
v=v
an=n

[音节]
ju=jv
e=ee
e=e;
er=er
ei=ei
eng=eg
en=en
ou=ou
o=oh
o=o;
ai=ai
ao=ao
a=ah
a=a;
ang=ag
an=an
shi=is
shi=ih
shen=ak
shen=it
zhen=ot
chen=ut
hen=hd
ken=kd
zhong=os
chong=us
kong=ks
jiong=js
de=dk
ce=cy
se=sy
re=ry
ze=zy
te=ty
ge=gi
li=ld
ji=jd
yi=ei
ni=nz
mi=mz
pi=pe
zhi=oh
biao=bb
niao=nb
nao=nb
miao=mb
mao=mb
diao=db
ting=tp
bing=bp
ying=ep
qing=qp
xing=xp
yin=ek
xin=xk
bin=bk
yu=eu
yao=ej
chao=ub
yong=el
chu=uf
shu=if
zhu=of
ku=kf
hu=hf
guang=gf
wei=wi
fei=fi
xiang=xh
qiang=qh
bang=bm
yang=eg
fang=fh
tang=tm
rang=rh
tian=tk
bian=bl
qian=qn
xian=xn
fa=fj
fa=f;
wa=wj
wa=w;
za=zm
zuan=zm
xuan=xm
suan=sm
duan=dm
guan=gm
quan=qm
yuan=em
tuan=tm
zhuan=vm
mo=mq
mo=mc
po=pb
po=pc
po=pq
chou=uq
zhou=oq
shou=iq
lou=lq
kou=kq
mou=mq
hou=hq
you=eo
you=yq
hua=ha
hua=ht
kua=ka
kua=kt
shua=it
zhua=ot
gua=ga
guai=gh
han=hw
kan=kw
yan=yw
chan=uw
shan=iw
zhan=ow
jia=ja
pia=pa
dia=dx
dia=dl
jie=je
mie=me
nie=ne
lie=lz
pie=pz
mu=mv
lu=lv
nu=nv
dui=d;
tui=t;
rui=r;
rui=rv
sui=s;
sui=sv
cui=c;
cui=cv
gui=g;
zui=z;
zui=zv
hui=hv
chui=uv
shui=iv
zhui=ov
kui=kv
qiu=qo
xiu=xo
lue=lb
yue=yb
yue=e;
jue=jb
men=md
er=eh
nai=nx
mai=mx
chai=ux
shai=ix
zhai=ox
pai=px
lai=lx
bei=b;
tun=t;
run=r;
qun=q;
zun=z;
xun=x;
cun=c;
gun=g;
sun=s;
dun=d;
zeng=zk
ceng=ck
deng=dv
leng=lk
geng=gk
yan=en
)RAW_TEXT";
    std::stringstream ss(csp);
    ShuangpinProfile profile(ss);
    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("fj", profile) == "fa");
    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("wj", profile) == "wa");
    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("zm", profile) == "za");
    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("bm", profile) == "bang");
    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("tm", profile) == "tang");
    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("tk", profile) == "tian");
    FCITX_ASSERT(PinyinEncoder::shuangpinToPinyin("jv", profile) == "ju");
    checkProfile(profile, true);
}

void checkAdvanceParsing2() {
    std::string colemak = R"RAW_TEXT(
[方案]
方案名称=小鹤测试（Colemak 韵母原位）

[零声母标识]
=O*

[声母]
ch=I
sh=U
zh=V

[韵母]
a=A
ai=S
an=N
ang=H
ao=C
e=F
ei=W
en=T
eng=D
er=R
i=U
ia=X
ian=M
iang=I
iao=K
ie=O
in=B
ing=E
iong=R
iu=Q
o=Y
ong=R
ou=Z
u=L
ua=X
uai=E
uan=P
uang=I
ue=G
ui=V
un=J
ve=G
uo=Y
)RAW_TEXT";
    std::stringstream ss(colemak);
    ShuangpinProfile profile(ss);
    const auto &table = profile.table();
    auto po = table.find("po");
    FCITX_ASSERT(po != table.end() && po->second.size() == 1 &&
                 po->second.begin()->first.toString() == "pie")
        << *po;
    auto ve = table.find("ve");
    FCITX_ASSERT(ve != table.end() && ve->second.size() == 1 &&
                 ve->second.begin()->first.toString() == "zhuai")
        << *ve;
    // ff -> e or f/h he
    auto ff = table.find("ff");
    FCITX_ASSERT(ff != table.end() &&
                 ff->second.count(
                     PinyinSyllable(PinyinInitial::Zero, PinyinFinal::E)) == 1)
        << *ff;
    // ft -> en
    auto ft = table.find("ft");
    FCITX_ASSERT(ft != table.end() &&
                 ft->second.count(
                     PinyinSyllable(PinyinInitial::Zero, PinyinFinal::EN)) == 1)
        << *ft;
    checkProfile(profile, false);
}

int main() {
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::Ziranma), false);
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::MS), false);
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::ABC), false);
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::Ziguang), false);
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::Zhongwenzhixing),
                 false);
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::PinyinJiajia),
                 false);
    checkProfile(ShuangpinProfile(ShuangpinBuiltinProfile::Xiaohe), false);
    checkABC();
    checkXiaoHe();
    checkZiranma();

    // wo jiu shi xiang ce shi yi xia
    checkSegments(PinyinEncoder::parseUserShuangpin(
                      "wojquixdceuiyixw",
                      ShuangpinProfile(ShuangpinBuiltinProfile::MS),
                      PinyinFuzzyFlag::None),
                  {{"wo", "jq", "ui", "xd", "ce", "ui", "yi", "xw"}});

    checkSegments(PinyinEncoder::parseUserShuangpin(
                      "aaaierorilah",
                      ShuangpinProfile(ShuangpinBuiltinProfile::Ziranma),
                      PinyinFuzzyFlag::None),
                  {{"aa", "ai", "er", "or", "il", "ah"}});

    checkSimpleParsing();
    checkAdvanceParsing();
    checkAdvanceParsing2();

    return 0;
}
