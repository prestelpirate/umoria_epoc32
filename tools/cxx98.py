#!/usr/bin/env python3
"""Mechanical C++11/14 -> C++98 rewrite of the umoria 5.7 sources so that the
EPOC SDK's gcc 3.0 can compile them. Run once on a pristine copy of src/.
Anything not handled here is fixed by hand (see git history)."""
import re, glob, os, sys

src = sys.argv[1] if len(sys.argv) > 1 else 'src'
files = glob.glob(os.path.join(src, '*.cpp')) + glob.glob(os.path.join(src, '*.h'))

def brace_calls(text, typename, newname):
    """TypeName{a, b} -> newname(a, b), respecting nested braces/parens."""
    out, i, pat = [], 0, typename + '{'
    while True:
        j = text.find(pat, i)
        if j < 0:
            out.append(text[i:]); break
        # must not be part of a longer identifier
        if j > 0 and (text[j-1].isalnum() or text[j-1] == '_'):
            out.append(text[i:j+len(pat)]); i = j + len(pat); continue
        k, depth = j + len(pat), 1
        while depth:
            c = text[k]
            if c == '{': depth += 1
            elif c == '}': depth -= 1
            k += 1
        inner = text[j+len(pat):k-1]
        inner = brace_calls(inner, typename, newname)
        out.append(text[i:j]); out.append('%s(%s)' % (newname, inner)); i = k
    return ''.join(out)

ENUM_RE = re.compile(r'enum class (\w+)(?:\s*:\s*[\w:]+)?\s*\{([^}]*)\};', re.S)
def enum_struct(m):
    name, body = m.group(1), m.group(2)
    return ('struct %s {\n    enum Value {%s};\n    Value v;\n    %s() {}\n'
            '    %s(Value a) : v(a) {}\n    operator Value() const { return v; }\n};') % (name, body, name, name)

# member defaults inside structs:  "type name = value;"  /  "type name[..]{};"
NSDMI_RE = re.compile(r'^(\s+(?!static\b|constexpr\b|return\b|typedef\b)[A-Za-z_][\w:<>]*(?:\s+[\w:<>]+)*[\s*&]+\w+(?:\[[^\]]*\])*)\s*(?:=\s*[^=;][^;]*|\{[^;]*\})\s*;', re.M)

for f in files:
    t = open(f, encoding='latin-1').read()
    o = t
    t = brace_calls(t, 'Coord_t', 'Coord_t_make')
    t = brace_calls(t, 'Dice_t', 'Dice_t_make')
    t = ENUM_RE.sub(enum_struct, t)
    if f.endswith('.h'):
        # only strip inside struct bodies: crude depth tracking
        lines, depth, res = t.split('\n'), 0, []
        stack = []
        for ln in lines:
            s = ln.strip()
            if stack and stack[-1] == 'struct' and NSDMI_RE.match(ln) and '(' not in ln.split('=')[0]:
                ln = NSDMI_RE.sub(r'\1;', ln)
            opens = ln.count('{'); closes = ln.count('}')
            kind = 'struct' if re.match(r'\s*(typedef\s+)?(struct|class)\b', ln) or re.match(r'\s*\}?\s*struct\s*\{', ln) or s == 'struct {' else 'other'
            for _ in range(opens): stack.append(kind)
            for _ in range(closes):
                if stack: stack.pop()
            res.append(ln)
        t = '\n'.join(res)
    # std:: prefix: provided globally by compat/prelude.h
    t = re.sub(r'\bstd::', '', t)
    # "auto x = <int literal>"  and "auto x = 0;" style loops
    t = re.sub(r'\bauto(\s+\w+\s*=\s*-?\d+\s*[;,])', r'int\1', t)
    if t != o:
        open(f, 'w', encoding='latin-1').write(t)
        print('converted', f)
