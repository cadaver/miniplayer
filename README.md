# Minimal C64 music player

9-rasterline player with limited featureset.

- Wave / pulse / filtertables with "next column" instead of jumps
- Delayed step, slide (indefinite) and vibrato commands in wavetable
- Pulse and filter tables are based on "destination value compare" instead of time counters
- Normal & legato instruments
- Keyoff command
- Change wavetable-pointer command
- Transpose
- Sound FX support, either overrides the channel completely (faster) or allows music to continue underneath
- Support for several music modules with the same player code, similar to NinjaTracker gamemusic mode.

Disadvantages:

- Only 1 frame of gateoff before new note (does not guarantee proper hard restart)
- Skips all realtime effects when reading new note data
- Pulse & filter tables can be only 127 steps, due to high bit of position indicating "init" step

Converter from GoatTracker 2 format included. See the player source (player.s) for data format details. Supported effects are 1,2,3,4 and F (no funktempo). Effect 3 (toneportamento) support is based on calculating the required slide duration, and may not work exactly in case of transposed patterns.

Use at own risk.

## License

Copyright (c) 2018 Lasse Öörni

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.