import sys, os

import imagemagick # (or wand?)
import inkscape

# Convert SVG images in bitmaps (only usefull for Windows - GDI?)

# August2111: noch in Vorbereitung - aber brauche ich das überhaupt, wenn ich die Bitmaps auf PNG umstelle (oder noch besser: auf SVG)?

if len(sys.argv) > 1:
    filename = sys.argv[1]

outdir = ''  # without this argument 2 write in the same directory like input file!
if len(sys.argv) > 2:
    outdir = sys.argv[2]
    outdir = outdir + '/'

