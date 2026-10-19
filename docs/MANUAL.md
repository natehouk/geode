# Geode 1.0 — User Manual

Geode is an algorithmic reverb with musical controls. It is designed to sit naturally in a mix: smooth and dense, without metallic ringing or phase problems. Typical uses:

- rooms, chambers, halls and churches for vocals, instruments and drums
- an effects-send reverb for a whole mix
- long washes, chorused tails and tempo-synced slaps for sound design

Geode has a stepless SPACE control that morphs across fourteen room models, plus Decay Rate, Brightness, Character, Distance and Stereo Width. It also has a six-band **decay rate EQ** that sets how long the reverb lasts at each frequency, a six-band **post EQ** that shapes the reverb's tone, and a predelay that can follow the host tempo.

Geode runs as VST3, AU, CLAP and VST2 on macOS, Windows and Linux.

**Contents**

1. [Installing](#1-installing)
2. [A quick tour](#2-a-quick-tour)
3. [How the reverb works](#3-how-the-reverb-works)
4. [The space controls](#4-the-space-controls)
5. [Mix, predelay and output](#5-mix-predelay-and-output)
6. [The display](#6-the-display)
7. [The decay rate EQ](#7-the-decay-rate-eq)
8. [The post EQ](#8-the-post-eq)
9. [Working with bands](#9-working-with-bands)
10. [Knobs](#10-knobs)
11. [A/B, undo and redo](#11-ab-undo-and-redo)
12. [MIDI](#12-midi)
13. [Presets](#13-presets)
14. [Help and hover tips](#14-help-and-hover-tips)
15. [Recipes](#15-recipes)
16. [Parameter reference](#16-parameter-reference)
17. [Troubleshooting](#17-troubleshooting)

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
  - the pills at the top left choose which EQ you are editing; ANALYZER at the top right switches the spectrum on or off
  - the current space and its decay time at 1 kHz are shown at the top
- **Main panel** (bottom left): SPACE, DECAY RATE, BRIGHTNESS, CHARACTER, DISTANCE, STEREO WIDTH; then PREDELAY with SYNC, MIX with LOCK, and OUTPUT.
- **Band panel** (bottom right): the selected EQ band: on/off, shape, slope (post EQ cuts), frequency, decay change or gain, and Q.

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
- **DECAY RATE** (50–200%) scales the decay time of the space: 50% halves it, 200% doubles it.
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

## 5. Mix, predelay and output

- **MIX** (0–100%) balances the dry signal against the reverb, with an equal-power crossfade. On an effects send, use 100%.
- **LOCK** keeps the current MIX when you load presets. You can then browse presets on a send (100%) or an insert without the balance jumping. LOCK is a view setting: it is saved with your project, but presets, A/B and undo don't change it.
- **PREDELAY** (0–500 ms) delays the start of the reverb. 10–40 ms keeps a voice or snare clear of its reverb.
- **SYNC** turns the PREDELAY knob into a note value at the host tempo: 1/64 to 1/2, straight, dotted (D) or triplet (T). It is capped at 500 ms.
- **OUTPUT** (−36 to +12 dB) sets the final level.

---

## 6. The display

- **The golden decay curve** shows how long the reverb really lasts at each frequency, as Geode builds it: the space's own treble and bass behaviour, BRIGHTNESS, DECAY RATE and the decay rate EQ. The right-hand scale is in seconds, centred on the decay time at mid frequencies.
- **The violet post EQ curve** shows the combined response of the post EQ bands, on the left-hand scale (±24 dB).
- **The analyzer** shows the spectrum of the reverb (after the post EQ). ANALYZER switches it on or off; it is a view setting.
- The text at the top shows the current space and its decay time at 1 kHz.

---

## 7. The decay rate EQ

The decay rate EQ changes how **long** the reverb lasts at some frequencies, not how loud it is. It replaces the bass/treble decay multipliers and crossovers of older reverbs with up to six freely placed bands.

- Click **DECAY RATE EQ** (top left of the display), then double-click the display to add a band.
- Drag a dot **up** to make the reverb ring longer at that frequency, **down** to make it die away sooner. The range is a quarter to four times the decay time; the band panel shows it as a percentage.
- Shapes:
  - **BELL** changes the decay around a frequency
  - **LO SHELF** and **HI SHELF** change it below or above a frequency
  - **NOTCH** makes the decay very short in a narrow band, for taming a ringing frequency in the tail
- Geode keeps the decay time under 40 seconds at every frequency, however the bands are set.

---

## 8. The post EQ

The post EQ shapes the tone of the reverb itself; the dry signal is untouched.

- Click **POST EQ**, then double-click the display to add a band.
- Shapes: **BELL**, **LO SHELF**, **HI SHELF**, **LO CUT** and **HI CUT**.
- Cut filters have slopes of 6, 12, 24, 48 or 96 dB per octave. For a cut, dragging the dot up and down sets its resonance (Q).
- A low cut at 150–300 Hz keeps a reverb from muddying the mix. A high cut or a high shelf darkens it.

---

## 9. Working with bands

Both EQs work the same way. You edit one EQ at a time: the other EQ's dots are shown small and dimmed. Clicking one of them switches to that EQ.

- **Add:** double-click the display, or click on the active curve. The new band follows the mouse until you let go.
- **Move:** drag a dot. Left and right set the frequency. Up and down set the decay change (decay EQ), gain (post EQ) or resonance (post EQ cuts). Hold **Shift** for fine adjustment.
- **Q:** turn the mouse wheel over a dot.
- **On/off:** Alt-click a dot, or use the switch in the band panel.
- **Remove:** right-click a dot, press Delete, or click the × in the band panel.
- The **band panel** edits the selected band: shape, slope, frequency, decay or gain, and Q.

---

## 10. Knobs

- **Drag** up or down; **Shift** for fine adjustment. **Grab the rim** of a knob to turn it in a circle.
- **Mouse wheel** turns a knob.
- **Double-click** to type a value, for example `45` (percent), `2.5k` or `120ms`. For SPACE, you can also type the start of a room name, for example `cath`.
- **Cmd/Ctrl-click** resets to the default.

---

## 11. A/B, undo and redo

- **A / B** hold two complete settings. Click the other letter to switch; the first time, B starts as a copy of A.
- **COPY** copies the current setting into the other slot.
- Every edit is an undo step, including new and deleted bands, preset loads and A/B switches.
  - Undo: Cmd/Ctrl-Z
  - Redo: Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y
- LOCK and ANALYZER are view settings: they are saved with your project, but presets, A/B and undo don't change them.

---

## 12. MIDI

- **MIDI Learn:**
  1. Click **LEARN**.
  2. Click a control. For a band control, select the band first.
  3. Move a knob or fader on your controller.
  4. Click **LEARN** again (or press Esc) to finish.

  Right-click a control while learning to unassign it. **CLEAR ALL ASSIGNMENTS** removes them all. Assignments are saved with your project.
- The **MIDI** button switches learned controllers off.

---

## 13. Presets

35 factory presets in five groups, plus your own:

| Group | Presets |
|---|---|
| Rooms | Ambience, Small Room, Vocal Booth, Studio Room, Live Room, Wood Room, Chamber |
| Halls & Churches | Small Hall, Concert Hall, Dark Hall, Bright Hall, Church, Cathedral, Vast Space |
| Vocals | Vocal Room, Bright Vocal, Vocal Hall (1/16), Ballad Tail, Soft Esses, Narration |
| Drums & Instruments | Drum Room, Snare Hall, Tight Kit, Piano Hall, Guitar Room, Strings Hall, Pad Wash |
| Creative | Chorus Verb, Echo Room, Near Infinite, Telephone Room, Dark Tail, Synced Slap (1/8), Mono Wash |

Presets set MIX for use as an insert (mostly 15–45%). On an effects send, set MIX to 100% and switch on LOCK.

- **SAVE CURRENT AS…** stores your own presets, in one of these folders:
  - macOS: `~/Library/Audio/Presets/Epsilon Audio/Geode`
  - Windows: `%APPDATA%\Epsilon Audio\Geode\Presets`
  - Linux: `~/.config/epsilon-audio/geode/presets`
- To delete one of your presets, right-click it in the browser and click again to confirm.

---

## 14. Help and hover tips

- Rest the pointer on any control, a dot, the display, a pill or the meters to see what it does and its current value.
- **HELP** opens the in-plugin pages. The switch at the bottom of the help window turns tips on or off.
- **FULL MANUAL (WEB)** opens this manual.

---

## 15. Recipes

- **Vocal hall:**
  1. Concert Hall, MIX 20–25%.
  2. PREDELAY SYNC at 1/16, or 20–40 ms.
  3. Post EQ low cut at about 250 Hz.
  4. Decay EQ high shelf at 6 kHz pulled down a little, so the esses don't hang in the tail.
- **Drum room:** Large Room, CHARACTER around 50% for audible reflections, DISTANCE 30–40%, post EQ low cut at 100 Hz.
- **Snare plate-style hall:** Concert Hall, DECAY RATE 80%, BRIGHTNESS 65%, decay EQ low shelf down (shorter lows), post EQ low cut at 180 Hz.
- **Tame a ringing tail:** add a decay EQ NOTCH at the ringing frequency and narrow it with the mouse wheel.
- **Long ambient wash:** Cathedral or Vast Cathedral, DECAY RATE 150–200%, CHARACTER 60–100% for a chorused tail, STEREO WIDTH 120%.
- **Reverb on a send:** MIX 100% with LOCK on, then browse presets freely.

---

## 16. Parameter reference

There are 92 parameters: 90 automatable controls and 2 view settings. New parameters are only ever added at the end, so sessions and automation keep working across versions.

- **Space:** Space, Decay Rate (50–200%), Brightness, Character, Distance (0–100%), Stereo Width (0–120%).
- **Mix:** Mix (0–100%), Output Level (−36 to +12 dB), Predelay (0–500 ms), Predelay Sync, Predelay Note (1/64 to 1/2), MIDI.
- **Decay EQ 1–6:** Used, Enabled, Shape (Bell, Low Shelf, High Shelf, Notch), Frequency (20 Hz – 20 kHz), Decay (−2 to +2 octaves of decay time), Q (0.1–10).
- **Post EQ 1–6:** Used, Enabled, Shape (Bell, Low Shelf, High Shelf, Low Cut, High Cut), Frequency (10 Hz – 30 kHz), Gain (±30 dB), Q (0.025–40), Slope (6–96 dB/oct).
- **View settings** (not automatable): Mix Lock, Analyzer.

Geode also reports its input and output levels to hosts that show plugin outputs.

---

## 17. Troubleshooting

- **Geode doesn't appear in my DAW.** Check the install folders above and rescan plugins. On macOS, run the `xattr` command from section 1.
- **The reverb is too loud or too quiet when I change presets.** Presets set MIX; switch on LOCK to keep yours.
- **The reverb muddies the mix.** Add a post EQ low cut at 150–300 Hz, or shorten the low end with a decay EQ low shelf.
- **The tail rings at one pitch.** Add a decay EQ NOTCH there.
- **Synced predelay doesn't follow the tempo.** The host must send its tempo to plugins; without it, Geode uses 120 BPM. Synced predelay is capped at 500 ms.
- **Latency:** Geode has none.
