#!/usr/bin/env python3
"""Second pass: auto declarations and range-based for loops -> C++98 + gcc __typeof__."""
import re, glob, os, sys
src = sys.argv[1] if len(sys.argv) > 1 else 'src'
CAST = re.compile(r'\bauto(\s+)(\w+)(\s*=\s*)\(\s*([\w:]+(?:\s*\*)?)\s*\)')
GEN = re.compile(r'\bauto(\s+)(\w+)\s*=\s*([^;]+);')
RFOR = re.compile(r'for\s*\(\s*((?:const\s+)?(?:auto|[\w:]+))\s*(&?)\s*(\w+)\s*:\s*([^)]+?)\s*\)\s*\{')
n = 0
def rfor(m):
    global n
    n += 1
    typ, ref, var, cont = m.group(1), m.group(2), m.group(3), m.group(4).strip()
    idx = '_rf%d' % n
    t = '__typeof__((%s)[0])' % cont if 'auto' in typ else typ.replace('const', '').strip()
    const = 'const ' if typ.startswith('const') else ''
    return ('for (size_t %s = 0; %s < sizeof(%s) / sizeof((%s)[0]); %s++) { %s%s %s%s = (%s)[%s];'
            % (idx, idx, cont, cont, idx, const, t, ref, var, cont, idx))
for f in glob.glob(os.path.join(src, '*.cpp')) + glob.glob(os.path.join(src, '*.h')):
    t = open(f, encoding='latin-1').read(); o = t
    t = RFOR.sub(rfor, t)
    t = CAST.sub(lambda m: '%s%s%s%s(%s)' % (m.group(4), m.group(1), m.group(2), m.group(3), m.group(4)), t)
    t = GEN.sub(lambda m: '__typeof__(%s)%s%s = %s;' % (m.group(3).strip(), m.group(1), m.group(2), m.group(3).strip()), t)
    if t != o:
        open(f, 'w', encoding='latin-1').write(t); print('converted', f)
