# Geode 2.0 — User Manual

Geode is an algorithmic reverb with musical controls. It is designed to sit naturally in a mix: smooth and dense, without metallic ringing or phase problems. Typical uses:

- rooms, chambers, halls and churches for vocals, instruments and drums
- an effects-send reverb for a whole mix
- long washes, chorused tails and tempo-synced slaps for sound design

Geode has:
- **Three algorithms:** Modern, Vintage and Plate.
- **A stepless SPACE control** that morphs across fourteen room models, plus Decay Rate (25–400%), Brightness, Character, Distance and Stereo Width.
- **Dynamics:** **Thickness**, **Ducking** and an **Auto Gate**.
- **Freeze.**
- **A six-band decay rate EQ** that sets how long the reverb lasts at each frequency.
- **A six-band post EQ** that shapes the reverb's tone, each band in stereo, mid, side, left or right.
- **A predelay** that can follow the host tempo.
- **Impulse response import:** Geode turns a WAV impulse response into its own settings.

Geode runs as VST3, AU, CLAP and VST2 on macOS, Windows and Linux.

**Contents**

1. [Installing](#1-installing)
2. [A quick tour](#2-a-quick-tour)
3. [How the reverb works](#3-how-the-reverb-works)
4. [The space controls](#4-the-space-controls)
5. [Algorithms](#5-algorithms)
6. [Mix, predelay and output](#6-mix-predelay-and-output)
7. [Thickness, ducking, auto gate and freeze](#7-thickness-ducking-auto-gate-and-freeze)
8. [The display](#8-the-display)
9. [The decay rate EQ](#9-the-decay-rate-eq)
10. [The post EQ](#10-the-post-eq)
11. [Working with bands](#11-working-with-bands)
12. [Importing an impulse response](#12-importing-an-impulse-response)
13. [Knobs](#13-knobs)
14. [A/B, undo and redo](#14-ab-undo-and-redo)
15. [MIDI](#15-midi)
16. [Presets](#16-presets)
17. [Help and hover tips](#17-help-and-hover-tips)
18. [Recipes](#18-recipes)
19. [Parameter reference](#19-parameter-reference)
20. [Compatibility with Geode 1.x](#20-compatibility-with-geode-1x)
21. [Troubleshooting](#21-troubleshooting)

---

## 1. Installing

Download the zip for your system from the project's **Releases** page and copy the plugin files:

| System | Copy | To |
|---|---|---|
| macOS | `Geode.vst3`, `Geode.component` (AU), `Geode.clap` | `~/Library/Audio/Plug-Ins/VST3`, `.../Components`, `.../CLAP` |
| Windows | the `Geode.vst3` folder, `Geode.clap` | `C:\Program Files\Common Files\VST3\`, `...\CLAP\` |
| Linux | `Geode.vst3`, `Geode.clap` | `~/.vst3/`, `~/.clap/` |

**macOS:** if the build isn't notarized and macOS blocks it, open Terminal and run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Geode*
```

Rescan plugins in your DAW. Geode appears under **Epsilon Audio** as a reverb, with a stereo input and output. Use it on mono tracks as usual.

---

## 2. A quick tour

![Geode](../screenshot.png)

- **Header:** preset browser, A/B and COPY, UNDO/REDO, LEARN (MIDI Learn) and MIDI on/off, HELP, and small input and output meters.
- **Display** (top):
  - the **golden curve** is the decay time at each frequency, read on the right-hand scale in seconds
  - the **violet curve** is the post EQ, read on the left-hand scale in dB
  - behind them, the spectrum of the reverb
  - the pills at the top left choose which EQ you are editing
  - at the top right:
    - IMPORT IR loads an impulse response
    - PIANO shows a keyboard scale
    - ANALYZER switches the spectrum on or off
  - the current space and its decay time at 1 kHz are shown at the top
- **Main panel** (bottom left), from top to bottom:
  - ALGORITHM and FREEZE
  - SPACE, DECAY RATE, BRIGHTNESS, CHARACTER, DISTANCE and STEREO WIDTH
  - PREDELAY with SYNC, THICKNESS, DUCKING, MIX with LOCK, and OUTPUT
  - AUTO GATE with its THRESHOLD and HOLD
- **Band panel** (bottom right): the selected EQ band:
  - on/off
  - shape
  - slope (post EQ cuts)
  - channel (post EQ)
  - frequency
  - decay change or gain
  - Q

---

## 3. How the reverb works

Geode builds a reverb the way a room does:

- **Predelay:** the time before the first reflections arrive. Your source stays clear, in front of its reverb.
- **Early reflections:** the first echoes from the walls, which tell your ear how big the room is and where the source stands in it.
- **The tail:** a dense network of sixteen interconnected delay lines that builds up and dies away smoothly. Each line has filters that set how fast each frequency fades. This is where the decay time, the brightness and the decay rate EQ come in.
- **Stereo, tone and post EQ** then shape the result, before it is mixed with the dry signal.

Every control changes smoothly while the music plays. You can turn SPACE from a small room to a cathedral without clicks.

---

## 4. The space controls

- **SPACE** fades steplessly between fourteen room models:

  | | | |
  |---|---|---|
  | Small Ambience | Small Room | Vocal Booth |
  | Studio Room | Medium Room | Large Room |
  | Chamber | Small Hall | Concert Hall |
  | Large Hall | Church | Large Church |
  | Cathedral | Vast Cathedral | |

  Each model has its own size, decay time (0.3 s to 8 s), early reflection pattern, and natural treble and bass behaviour. Between two models, Geode blends them. The knob shows the nearest model's name.
- **DECAY RATE** (25–400%) scales the decay time of the space: 25% makes it a quarter as long, 400% four times as long.
- **BRIGHTNESS** (0–100%) sets the tone of the reverb, and how fast the high frequencies die away compared with the rest. 50% is natural.
- **CHARACTER** (0–100%):
  - left: a clean, smooth, transparent decay
  - middle: a livelier sound with distinct reflections and echoes
  - right: an over-modulated, chorus-like tail
- **DISTANCE** (0–100%) places the source in the room:
  - close (left): strong early reflections, a brighter and more direct sound
  - far (right): softer reflections, more air absorption, and a tail that starts later
- **STEREO WIDTH** (0–120%): 0% is mono, 100% natural stereo, 120% extra wide.

---

## 5. Algorithms

All three algorithms share every control. You can switch between them while playing.

| Algorithm | Sound | Good for |
|---|---|---|
| **Modern** | Geode's own reverb: clean, dense and natural. This is the 1.x sound. | Everything |
| **Vintage** | Recalls the digital reverb units of the late 1980s and 1990s:<br>• a limited bandwidth (about 8 kHz) and darker tails<br>• grainier diffusion<br>• a slower, deeper chorus in the tail<br>• a short (14-bit) word length | Synths, pads, 80s and 90s productions, lo-fi |
| **Plate** | The sound of a plate:<br>• bright, dense and slightly metallic<br>• no room reflections<br>• an immediate build-up and highs that last<br>SPACE still sets the decay time; Distance and the room models' reflections don't apply. | Vocals, snares, guitars |

---

## 6. Mix, predelay and output

- **MIX** (0–100%) balances the dry signal against the reverb, with an equal-power crossfade. On an effects send, use 100%.
- **LOCK** keeps the current MIX when you load presets. You can then browse presets on a send (100%) or an insert without the balance jumping. LOCK is a view setting: it is saved with your project, but presets, A/B and undo don't change it.
- **PREDELAY** (0–500 ms) delays the start of the reverb. 10–40 ms keeps a voice or snare clear of its reverb.
- **SYNC** turns the PREDELAY knob into a note value at the host tempo: 1/64 to 1/2, straight, dotted (D) or triplet (T). It is capped at 500 ms.
- **OUTPUT** (−36 to +12 dB) sets the final level.

---

## 7. Thickness, ducking, auto gate and freeze

- **THICKNESS** (0–100%) adds saturation and compression to the reverb:
  - It sounds denser and fuller, and holds its level as it decays.
  - Geode compensates the level, so turning it up changes the density, not the loudness.
- **DUCKING** (0–100%) makes the reverb step back while the dry signal plays, and come back up in the gaps:
  - At 100%, the reverb is turned down by up to 30 dB under loud input.
  - It returns within a fraction of a second once the input goes quiet.
  - This gives you a big reverb that doesn't cloud the vocal.
- **AUTO GATE** cuts the reverb off when the input stops:
  - It opens as soon as the input rises above **THRESHOLD** (−80 to 0 dB).
  - It closes **HOLD** (10 ms to 2 s) after the input falls below it, with a short fade.
  - Use it for the classic gated reverb on drums, or just for tidy tails.
- **FREEZE** holds the reverb as it is, indefinitely, and keeps new sound out of it. Release it and the tail decays normally. Good for pads, transitions and sound design.

---

## 8. The display

- **The golden decay curve** shows how long the reverb really lasts at each frequency, as Geode builds it: the space's own treble and bass behaviour, BRIGHTNESS, DECAY RATE and the decay rate EQ. The right-hand scale is in seconds, centred on the decay time at mid frequencies.
- **The violet post EQ curve** shows the combined response of the post EQ bands, on the left-hand scale (±24 dB).
- **The analyzer** shows the spectrum of the reverb (after the post EQ). ANALYZER switches it on or off; it is a view setting.
- **PIANO** draws a keyboard along the frequency scale and labels every C (A4 = 440 Hz), so you can place bands on notes. It is a view setting.
- The text at the top shows the current space and its decay time at 1 kHz.

---

## 9. The decay rate EQ

The decay rate EQ changes how **long** the reverb lasts at some frequencies, not how loud it is. It replaces the bass/treble decay multipliers and crossovers of older reverbs with up to six freely placed bands.

- Click **DECAY RATE EQ** (top left of the display), then double-click the display to add a band.
- Drag a dot **up** to make the reverb ring longer at that frequency, **down** to make it die away sooner. The range is a quarter to four times the decay time; the band panel shows it as a percentage.
- Shapes:
  - **BELL** changes the decay around a frequency
  - **LO SHELF** and **HI SHELF** change it below or above a frequency
  - **NOTCH** makes the decay very short in a narrow band, for taming a ringing frequency in the tail
- Geode keeps the decay time under 40 seconds at every frequency, however the bands are set.

---

## 10. The post EQ

The post EQ shapes the tone of the reverb itself; the dry signal is untouched.

- Click **POST EQ**, then double-click the display to add a band.
- Shapes: **BELL**, **LO SHELF**, **HI SHELF**, **LO CUT** and **HI CUT**.
- Cut filters have slopes of 6, 12, 24, 48 or 96 dB per octave. For a cut, dragging the dot up and down sets its resonance (Q).
- **CHANNEL** sets what each band works on:
  - **STEREO**: both channels
  - **MID**: the centre of the reverb
  - **SIDE**: its width
  - **LEFT** or **RIGHT** only

  For example, a low cut on the SIDE keeps the low end of the reverb in the middle.
- A low cut at 150–300 Hz keeps a reverb from muddying the mix. A high cut or a high shelf darkens it.

---

## 11. Working with bands

Both EQs work the same way. You edit one EQ at a time: the other EQ's dots are shown small and dimmed. Clicking one of them switches to that EQ.

- **Add:** double-click the display, or click on the active curve. The new band follows the mouse until you let go.
- **Move:** drag a dot. Left and right set the frequency. Up and down set the decay change (decay EQ), gain (post EQ) or resonance (post EQ cuts). Hold **Shift** for fine adjustment.
- **Q:** turn the mouse wheel over a dot.
- **On/off:** Alt-click a dot, or use the switch in the band panel.
- **Remove:** right-click a dot, press Delete, or click the × in the band panel.
- The **band panel** edits the selected band: shape, slope, channel, frequency, decay or gain, and Q.

---

## 12. Importing an impulse response

**IMPORT IR** (above the display) loads an impulse response, a WAV file, and sets Geode up to sound like it. Supported files are 16-, 24- or 32-bit PCM or 32/64-bit float, mono or stereo, at any sample rate; Geode reads up to 30 seconds.

Geode analyses the file and works out:

| From the IR | Geode setting |
|---|---|
| The decay time in the mids (500 Hz – 1 kHz) | SPACE (the room model whose natural decay is closest) and DECAY RATE |
| The decay time in each octave band, 125 Hz to 8 kHz | A six-band DECAY RATE EQ, refined by rendering Geode and measuring it again |
| The gap between the direct sound and the reverb | PREDELAY |
| The balance of early and late sound (clarity, C50) | DISTANCE |
| The correlation of left and right | STEREO WIDTH (0% for a mono file) |
| The remaining difference in tone | A six-band POST EQ |

How it works, and what it means for the result:
- **It is a match, not convolution.** The result is ordinary Geode settings, which you can then tweak, automate and save.
- **Direct sound:** if the file starts with a direct sound (a strong first spike), that part is left to the dry signal, and the predelay is measured from it.
- **One undo step:** the import counts as a single edit, so undo brings back what you had.
- **MIX is unchanged.**

In tests, Geode matches the decay time in every octave band to within 20%, both for its own impulse responses and for synthetic rooms. When it re-imports its own impulse responses, the spectrum matches to within 3 dB RMS.

---

## 13. Knobs

- **Drag** up or down; **Shift** for fine adjustment. **Grab the rim** of a knob to turn it in a circle.
- **Mouse wheel** turns a knob.
- **Double-click** to type a value, for example `45` (percent), `2.5k` or `120ms`. For SPACE, you can also type the start of a room name, for example `cath`.
- **Cmd/Ctrl-click** resets to the default.

---

## 14. A/B, undo and redo

- **A / B** hold two complete settings. Click the other letter to switch; the first time, B starts as a copy of A.
- **COPY** copies the current setting into the other slot.
- Every edit is an undo step, including new and deleted bands, preset loads and A/B switches.
  - Undo: Cmd/Ctrl-Z
  - Redo: Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y
- LOCK, ANALYZER and PIANO are view settings: they are saved with your project, but presets, A/B and undo don't change them.

---

## 15. MIDI

- **MIDI Learn:**
  1. Click **LEARN**.
  2. Click a control. For a band control, select the band first.
  3. Move a knob or fader on your controller.
  4. Click **LEARN** again (or press Esc) to finish.

  Right-click a control while learning to unassign it. **CLEAR ALL ASSIGNMENTS** removes them all. Assignments are saved with your project.
- The **MIDI** button switches learned controllers off.

---

## 16. Presets

46 factory presets in five groups, plus your own:

| Group | Presets |
|---|---|
| Rooms | Ambience, Small Room, Vocal Booth, Studio Room, Live Room, Wood Room, Chamber; Vintage Chamber |
| Halls & Churches | Small Hall, Concert Hall, Dark Hall, Bright Hall, Church, Cathedral, Vast Space; 80s Digital Hall, Endless Hall |
| Vocals | Vocal Room, Bright Vocal, Vocal Hall (1/16), Ballad Tail, Soft Esses, Narration; Vocal Plate, Ducked Vocal Hall |
| Drums & Instruments | Drum Room, Snare Hall, Tight Kit, Piano Hall, Guitar Room, Strings Hall, Pad Wash; Drum Plate, Gated Snare, Thick Drum Room |
| Creative | Chorus Verb, Echo Room, Near Infinite, Telephone Room, Dark Tail, Synced Slap (1/8), Mono Wash; Lo-Fi Verb, Ducked Wash, Short Burst |

The presets after the semicolon are new in 2.0. They come after the 1.x presets, so 1.x program numbers stay the same.

**The preset browser:**
- **Search:** type while it is open to search preset names and categories. Backspace edits the search; Esc clears it, then closes the browser.
- **Favourites:** click the star next to a preset (it appears when you point at it) to add it to the FAVOURITES column. Click again to remove it. Favourites are kept with your user presets.

Presets set MIX for use as an insert (mostly 15–45%). On an effects send, set MIX to 100% and switch on LOCK.

- **SAVE CURRENT AS…** stores your own presets, in one of these folders:
  - macOS: `~/Library/Audio/Presets/Epsilon Audio/Geode`
  - Windows: `%APPDATA%\Epsilon Audio\Geode\Presets`
  - Linux: `~/.config/epsilon-audio/geode/presets`
- To delete one of your presets, right-click it in the browser and click again to confirm.

---

## 17. Help and hover tips

- Rest the pointer on any control, a dot, the display, a pill, IMPORT IR or the meters to see what it does and its current value.
- **HELP** opens the in-plugin pages. The switch at the bottom of the help window turns tips on or off.
- **FULL MANUAL (WEB)** opens this manual.

---

## 18. Recipes

- **Vocal hall:**
  1. Concert Hall, MIX 20–25%.
  2. PREDELAY SYNC at 1/16, or 20–40 ms.
  3. Post EQ low cut at about 250 Hz.
  4. Decay EQ high shelf at 6 kHz pulled down a little, so the esses don't hang in the tail.
- **Drum room:** Large Room, CHARACTER around 50% for audible reflections, DISTANCE 30–40%, post EQ low cut at 100 Hz.
- **Snare plate-style hall:** Concert Hall, DECAY RATE 80%, BRIGHTNESS 65%, decay EQ low shelf down (shorter lows), post EQ low cut at 180 Hz.
- **Tame a ringing tail:** add a decay EQ NOTCH at the ringing frequency and narrow it with the mouse wheel.
- **Long ambient wash:** Cathedral or Vast Cathedral, DECAY RATE 200–400%, CHARACTER 60–100% for a chorused tail, STEREO WIDTH 120%. Add DUCKING 60–80% so it blooms between phrases.
- **Vocal plate:** PLATE, SPACE around Medium Room, BRIGHTNESS 65%, PREDELAY 20 ms, post EQ low cut at 250 Hz.
- **Gated drums:** Small Hall, AUTO GATE on, THRESHOLD around −35 dB, HOLD 150–250 ms, THICKNESS 40%.
- **80s synth hall:** VINTAGE, Concert Hall, CHARACTER 45%, DECAY RATE 120%.
- **Match a room you recorded:** record a clap or a sine sweep (deconvolved) in the room, save it as a WAV, and import it with IMPORT IR.
- **Freeze a chord into a pad:** play the chord, press FREEZE while it rings, and play over it.
- **Reverb on a send:** MIX 100% with LOCK on, then browse presets freely.

---

## 19. Parameter reference

There are 106 parameters: 103 automatable controls and 3 view settings. New parameters are only ever added at the end, so sessions and automation keep working across versions.

- **Space:** Space, Decay Rate (25–400%), Brightness, Character, Distance (0–100%), Stereo Width (0–120%).
- **2.0:**
  - Algorithm (Modern, Vintage, Plate)
  - Thickness and Ducking (0–100%)
  - Auto Gate, with Gate Threshold (−80 to 0 dB) and Gate Hold (10 ms – 2 s)
  - Freeze
  - Post EQ 1–6 Channel (Stereo, Mid, Side, Left, Right)
- **Mix:** Mix (0–100%), Output Level (−36 to +12 dB), Predelay (0–500 ms), Predelay Sync, Predelay Note (1/64 to 1/2), MIDI.
- **Decay EQ 1–6:** Used, Enabled, Shape (Bell, Low Shelf, High Shelf, Notch), Frequency (20 Hz – 20 kHz), Decay (−2 to +2 octaves of decay time), Q (0.1–10).
- **Post EQ 1–6:** Used, Enabled, Shape (Bell, Low Shelf, High Shelf, Low Cut, High Cut), Frequency (10 Hz – 30 kHz), Gain (±30 dB), Q (0.025–40), Slope (6–96 dB/oct).
- **View settings** (not automatable): Mix Lock, Analyzer, Piano Display.

Geode also reports its input and output levels to hosts that show plugin outputs.

---

## 20. Compatibility with Geode 1.x

Geode 2.0 is the same plugin (same IDs), version 2.0.
- **Sessions load and sound the same.** Every 1.x parameter keeps its number and meaning. The 2.0 parameters come after them, with defaults that change nothing: the Modern algorithm, no thickness, ducking, gate or freeze, and stereo post EQ bands. An automated test plays random 1.x settings, with changes mid-stream, through a frozen copy of the 1.0 engine and through 2.0. The outputs are bit-identical.
- **Decay Rate** now spans 25–400% instead of 50–200%. Stored values keep their meaning. Hosts that store automation as a 0–1 position along a control's range may map existing automation lanes for Decay Rate differently, so check them after upgrading.
- **Factory presets** keep their program numbers. **User presets** and **MIDI Learn** assignments carry over.
- **Window:** a little taller by default (1100 × 800), and it still resizes freely.

---

## 21. Troubleshooting

- **Geode doesn't appear in my DAW.** Check the install folders above and rescan plugins. On macOS, run the `xattr` command from section 1.
- **The reverb is too loud or too quiet when I change presets.** Presets set MIX; switch on LOCK to keep yours.
- **The reverb muddies the mix.** Add a post EQ low cut at 150–300 Hz, or shorten the low end with a decay EQ low shelf.
- **The tail rings at one pitch.** Add a decay EQ NOTCH there.
- **Synced predelay doesn't follow the tempo.** The host must send its tempo to plugins; without it, Geode uses 120 BPM. Synced predelay is capped at 500 ms.
- **The reverb stopped changing, or new sound doesn't reach it.** FREEZE is on (FROZEN shows in the display). Switch it off.
- **The reverb disappears between notes.** AUTO GATE is on; lower its THRESHOLD or lengthen HOLD. Or DUCKING is high, which is meant to turn the reverb down while the input plays.
- **IMPORT IR says it could not measure a decay.** The file has to contain a reverb tail at least 50 ms long, rising clearly above its noise floor. Very short or very noisy files can't be analysed.
- **The imported sound is close but not identical.** Geode matches the decay and tone with its own algorithm; it does not convolve. Adjust CHARACTER, DISTANCE or the EQs to taste.
- **Latency:** Geode has none.
