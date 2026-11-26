#!/bin/sh
# Regenerates firmware/src/fonts/*.h. Pass the directory holding the JetBrains Mono
# v2.304 TTFs (see fontgen.py for the download URL). Sizes are display-readability
# choices; revisit after the step 5 sunlight test.
set -eu
TTF_DIR=${1:?usage: make_fonts.sh <dir with JetBrainsMono-*.ttf>}
cd "$(dirname "$0")/.."
ASCII=$(python3 -c "print(''.join(chr(c) for c in range(32, 127)) + '°')")
python3 tools/fontgen.py "$TTF_DIR/JetBrainsMono-Bold.ttf" 20 kFontText "$ASCII" src/fonts/font_text.h
python3 tools/fontgen.py "$TTF_DIR/JetBrainsMono-ExtraBold.ttf" 40 kFontBig " -.0123456789hkm" src/fonts/font_big.h
python3 tools/fontgen.py "$TTF_DIR/JetBrainsMono-ExtraBold.ttf" 72 kFontClock "-0123456789:" src/fonts/font_clock.h
