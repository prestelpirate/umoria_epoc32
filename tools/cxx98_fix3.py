#!/usr/bin/env python3
"""Third pass: plain enums used as Name::Value, <cxxx> headers, CTRL_KEY, CNIL,
enum-wrapper int constructors."""
import re, glob, os, sys
src = sys.argv[1] if len(sys.argv) > 1 else 'src'
files = glob.glob(os.path.join(src, '*.cpp')) + glob.glob(os.path.join(src, '*.h'))
alltext = ''.join(open(f, encoding='latin-1').read() for f in files)
# plain enums referenced with Name:: qualification
scoped = set(re.findall(r'\benum (\w+)\s*\{', alltext)) & set(re.findall(r'\b(\w+)::', alltext))
print('scoped plain enums:', sorted(scoped))
for f in files:
    t = open(f, encoding='latin-1').read(); o = t
    for n in scoped:
        t = re.sub(r'\benum %s\s*\{([^}]*)\};' % n, r'struct %s {\n    enum Value {\1};\n};' % n, t)
        # the enum type name used as a type -> int
        t = re.sub(r'\b%s\b(?!\s*::)(?=\s+[\w&*])' % n, 'int', t)
    t = re.sub(r'#include <c(ctype|errno|stdint|stdio|stdlib|string|time|assert|limits)>', r'#include <\1.h>', t)
    t = re.sub(r'constexpr char CTRL_KEY\(char x\) \{\s*return[^;]*;\s*\}', '#define CTRL_KEY(x) ((char) ((x) & 0x1F))', t)
    t = re.sub(r'\n    Value v;\n    (\w+)\(\) \{\}\n', lambda m: '\n    Value v;\n    %s() {}\n    explicit %s(int a) : v((Value) a) {}\n' % (m.group(1), m.group(1)), t)
    if t != o:
        open(f, 'w', encoding='latin-1').write(t); print('fixed', f)
