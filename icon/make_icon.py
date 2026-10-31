#!/usr/bin/env python3
"""umoria icon.jpg -> EPOC icon bitmaps (24/32/48 px colour + 1-bit mask).
The source is a white glyph on a black square; the whole square is kept
(mask fully opaque, black = shown in EPOC masks)."""
from PIL import Image
import sys, os
src, out = sys.argv[1], sys.argv[2]
im = Image.open(src).convert('RGB')
for s in (24, 32, 48):
    im.resize((s, s), Image.LANCZOS).save(os.path.join(out, 'icon%d.bmp' % s))
    Image.new('1', (s, s), 0).save(os.path.join(out, 'mask%d.bmp' % s))
print('ok')
