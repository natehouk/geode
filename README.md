# Geode

**An algorithmic reverb with Modern, Vintage and Plate algorithms, a stepless space control, a decay rate EQ, a post EQ and impulse response import, for macOS, Windows and Linux (VST3 · AU · CLAP · VST2).**

Part of **[Epsilon Audio](https://github.com/natehouk/epsilon-plugins)**, a family of free, open-source plugins: Monolith, Caldera, Obsidian, Strata, Basalt, Gneiss, Mica, Flint, Granite, Magma, Schist, Slate, Talc, Geode and Malachite.

![Geode](screenshot.png)

Geode is a clean-room design with the feature set of FabFilter Pro-R 2. Geode 1.x matched the original Pro-R (November 2016). Version 2.0 adds:
- the Vintage and Plate algorithms
- Decay Rate from 25% to 400%
- Thickness, Ducking, Auto Gate and Freeze
- mid/side and left/right post EQ bands
- a piano display scale
- a preset browser with search and favourites
- impulse response import, which turns a WAV impulse response into Geode settings

It was written from scratch, working only from FabFilter's public product pages, release announcements and news coverage. No FabFilter code, binaries, presets, artwork or trademarks were used.

It is part of a family of plugins:
- [Monolith](https://github.com/natehouk/monolith) (synth)
- [Caldera](https://github.com/natehouk/caldera) (filter)
- [Obsidian](https://github.com/natehouk/obsidian) (polysynth)
- [Strata](https://github.com/natehouk/strata) (tape delay)
- [Basalt](https://github.com/natehouk/basalt) (live dual filter)
- [Gneiss](https://github.com/natehouk/gneiss) (compressor)
- [Mica](https://github.com/natehouk/mica) (equalizer)
- [Flint](https://github.com/natehouk/flint) (small filter)
- [Granite](https://github.com/natehouk/granite) (limiter)
- [Magma](https://github.com/natehouk/magma) (multiband distortion)
- [Schist](https://github.com/natehouk/schist) (multiband dynamics)
- [Slate](https://github.com/natehouk/slate) (gate / expander)
- [Talc](https://github.com/natehouk/talc) (de-esser)
- **Geode**, a hollow stone lined with crystals: a resonant space

## Download

Get the latest build from [**Releases**](../../releases):

| Platform | File | Contains |
|---|---|---|
| macOS 10.13+ (Apple Silicon + Intel) | `Geode-macos-universal.zip` | VST3, AU, CLAP, manual |
| Windows x64 | `Geode-windows-x64.zip` | VST3, CLAP, VST2, manual |
| Linux x64 | `Geode-linux-x64.zip` | VST3, CLAP, VST2, manual |

### Install

| Platform | Copy | To |
|---|---|---|
| macOS | `Geode.vst3` / `Geode.component` / `Geode.clap` | `~/Library/Audio/Plug-Ins/VST3` / `Components` / `CLAP` |
| Windows | `Geode.vst3` (folder) / `Geode.clap` | `C:\Program Files\Common Files\VST3\` / `CLAP\` |
| Linux | `Geode.vst3` / `Geode.clap` | `~/.vst3/` / `~/.clap/` |

**macOS:** builds are notarized once the project's Apple signing credentials are set up. Until then they are ad-hoc signed; after copying, run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Geode*
```

## Features

**Algorithms**
- **Modern:** Geode's clean, dense, natural reverb (the 1.x sound).
- **Vintage:** the sound of late-80s and 90s digital units. It is band-limited, with grainier diffusion, a slower and deeper chorus, and a 14-bit word length.
- **Plate:** bright, dense and metallic, with no room reflections and an immediate build-up.

**The space**
- **Space:** a stepless control that morphs between fourteen room models, from Small Ambience to Vast Cathedral, without clicks. The decay time follows.
- **Decay Rate** scales the decay time of the space from 25% to 400%.
- **Brightness** sets the tone, and how fast the highs die away.
- **Character** goes from a clean decay, through lively reflections and echoes, to an over-modulated, chorus-like tail.
- **Distance** moves the source from close (strong early reflections) to far (soft reflections, air, a later tail).
- **Stereo Width** from mono through stereo to extra wide (120%).

**Dynamics and freeze**
- **Thickness:** saturation and compression, level-compensated.
- **Ducking:** up to 30 dB under the dry signal.
- **Auto Gate:** a threshold and hold time, for gated reverbs and tidy tails.
- **Freeze:** an infinite, lossless hold that keeps new sound out.

**EQ**
- **Decay rate EQ:** six bands (bell, shelves, notch) that set how long the reverb lasts at each frequency, from a quarter to four times the decay time, without changing its level.
- **Post EQ:** six bands (bell, shelves, low and high cut up to 96 dB/oct) that shape the reverb only. Each band works on stereo, mid, side, left or right.

**Impulse response import**
- Load a WAV impulse response, and Geode works out:
  - space and decay rate
  - a decay rate EQ matching the decay time per octave
  - predelay (a direct sound is left to the dry signal)
  - distance and width
  - a post EQ for the tone

  The result is ordinary Geode settings: a match, not convolution.
- In tests, the decay time per octave matches within 20%, and Geode's own impulse responses come back within 3 dB of spectrum.

**Mix and workflow**
- Mix with **mix lock** for browsing presets on a send, output level.
- Predelay 0-500 ms, or **tempo-synced** (1/64 to 1/2, straight, dotted and triplet).
- **Interactive display** showing:
  - the real decay time at each frequency (golden curve, in seconds)
  - the post EQ (violet curve)
  - a live spectrum of the reverb
  - an optional **piano scale**
- Zero latency.
- A/B with copy, undo/redo for every edit, MIDI Learn.
- Smoothed, sample-accurate automation.
- Resizable interface.
- **46 factory presets** in five categories, plus user presets. The **preset browser** has search and favourites.
- **Hover tips** everywhere, a **HELP window**, and a **[full manual](docs/MANUAL.md)** (also in every release zip as `Geode-Manual.html`).

![Help](screenshot-help.png)

### Compared with FabFilter Pro-R 2

| Pro-R 2 | Geode |
|---|---|
| Space: stepless control fading between room models, with a matching decay time | ✓ fourteen models, from Small Ambience to Vast Cathedral |
| Vintage algorithm (1980s-90s digital units) and Plate algorithm | ✓ |
| Decay Rate 25-400% | ✓ |
| Brightness, Character, Distance, Stereo Width | ✓ |
| Thickness (saturation and compression) | ✓ |
| Ducking, Auto Gate | ✓ (gate with threshold and hold) |
| Freeze | ✓ |
| Mix with Lock Mix; predelay 0-500 ms with tempo sync | ✓ |
| Decay Rate EQ: six bands | ✓ bell, shelves, notch |
| Post EQ: six bands, each mid/side or L/R | ✓ stereo, mid, side, left, right |
| Impulse response import, analysed into settings, decay rate and output EQ | ✓ |
| Real-time spectrum analyzer that also shows decay time per frequency | ✓ |
| Piano display scale | ✓ |
| Preset browser with search, tags, favourites and filtering | ✓ search (names and categories) and favourites |
| MIDI Learn, undo/redo, A/B, smooth parameter changes, help with interactive hints | ✓ hover tips, help pages and full manual |
| Resizable interface, full-screen mode | Freely resizable interface |
| Mono, surround and Dolby Atmos (up to 9.1.6) | Stereo only |
| AAX, AudioSuite, Pro Tools control surfaces | Not supported (needs Avid's SDK and signing) |
| iPad (AUv3) | Not supported |

## Build from source

The plugin is built on [DPF](https://github.com/DISTRHO/DPF):

```
git clone --recursive https://github.com/DISTRHO/DPF.git dpf
make -C plugins/Geode                    # Linux / macOS (macOS also builds the AU)
make -C plugins/Geode WINDOWS=true CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++
```

GitHub Actions builds all three platforms on every push. The macOS build is universal. It is signed and notarized when the Developer ID secrets are set (`MACOS_CERT_P12`, `MACOS_CERT_PASSWORD`, `NOTARY_KEY`, `NOTARY_KEY_ID`, `NOTARY_ISSUER`), and ad-hoc signed otherwise. A `v*` tag, or a release published on GitHub, attaches the zips to that release.

## Tests

```
cd tests
g++ -O2 -std=c++17 -I../plugins/Geode features.cpp -o features && ./features    # 1.x behaviour: RT60 vs the displayed decay time, decay EQ, notch, brightness,
                                                                               # predelay, width, post EQ, level across spaces, stability, click-free sweeps
g++ -O2 -std=c++17 -I../plugins/Geode features2.cpp -o features2 && ./features2 # 2.0: decay 25-400%, algorithms, freeze, thickness, ducking, auto gate,
                                                                               # post EQ channels, random settings
g++ -O2 -std=c++17 -I../plugins/Geode compat.cpp -o compat && ./compat          # random 1.x settings through the frozen 1.0 engine and 2.0: bit-identical
g++ -O2 -std=c++17 -I../plugins/Geode irmatch.cpp -o irmatch && ./irmatch       # WAV formats; matching Geode's own IRs and synthetic rooms
g++ -O2 -std=c++17 -I../plugins/Geode fuzz.cpp -o fuzz && ./fuzz                # random parameters and automation at 44.1-192 kHz: always finite
g++ -O2 -std=c++17 -I../plugins/Geode presets.cpp -o presets && ./presets       # a noise burst through every preset: level, peak and tail
g++ -O3 -std=c++17 -I../plugins/Geode bench.cpp -o bench && ./bench             # CPU
```

CI runs all of these except the benchmark on every push.

CPU use, as a share of one core at 48 kHz:

| Configuration | CPU |
|---|---|
| Default | about 4% |
| With all twelve EQ bands | about 7% |
| Vintage with thickness, ducking and gate | about 8% |

Release checks:
- Steinberg's VST3 validator passes 47/47.
- The interface is exercised with scripted input and a live test mix. This covers:
  - algorithms and the new knobs
  - post EQ channels and the piano scale
  - preset search and favourites
  - importing an impulse response through the file browser

## Source layout

| File | Contents |
|---|---|
| `plugins/Geode/GeodeEngine.hpp` | All DSP. Header-only. It covers:<br>• room models and algorithms<br>• predelay, early reflections and diffusion<br>• the 16-line feedback delay network and its decay filters<br>• width, tone, thickness and post EQ<br>• ducking, gate and freeze |
| `plugins/Geode/IrMatch.hpp` | Impulse response import. It covers:<br>• WAV reading<br>• decay and spectrum analysis<br>• fitting the settings, decay rate EQ and post EQ |
| `plugins/Geode/ParamDefs.hpp` | Parameter table, labels, value mapping |
| `plugins/Geode/Presets.hpp` | Factory presets |
| `plugins/Geode/UserPresets.hpp` | User preset files and favourites |
| `plugins/Geode/Help.hpp` | Hover tip text and in-plugin help pages |
| `plugins/Geode/Fft.hpp` | FFT for the analyzer and IR import |
| `plugins/Geode/GeodePlugin.hpp/.cpp` | Plugin wrapper: parameters, tempo, MIDI Learn |
| `plugins/Geode/GeodeUI.cpp` | Custom vector interface (NanoVG) |
| `tests/legacy/` | Frozen 1.0 engine, used by the compatibility test |
| `docs/MANUAL.md` | User manual (`Geode-Manual.html` is generated from it with pandoc) |

New parameters, styles and presets are only ever appended, so saved sessions and automation keep working across versions.

## Changelog

- **2.0:** Brings Geode up to Pro-R 2. New in this version:
  - Vintage and Plate algorithms.
  - Decay Rate 25-400%.
  - Thickness, Ducking, Auto Gate and Freeze.
  - Post EQ bands on mid, side, left or right.
  - Impulse response import.
  - Piano display scale.
  - Preset search and favourites.
  - Eleven new presets.

  Sessions, automation and presets from 1.x load and sound the same. A test checks this against the frozen 1.0 engine.

  Decay Rate now spans a wider range. In hosts that store automation as a 0-1 position, check those lanes after upgrading.
- **1.0:** First release.

## License

[MIT](LICENSE). Built with [DPF](https://github.com/DISTRHO/DPF) and its bundled NanoVG, which are under the ISC license. `plugins/Geode/ResizeHandle.hpp` comes from DPF's examples and keeps its ISC notice.

"FabFilter", "FabFilter Pro-R" and "Pro-R 2" are trademarks of FabFilter Software Instruments. Geode is not affiliated with or endorsed by FabFilter.
