// Geode — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "GeodeEngine.hpp"

namespace geode {

static const char* const kManualURL = "https://github.com/natehouk/geode/blob/main/docs/MANUAL.md";

inline const char* paramTip(uint32_t p) {
    switch (p) {
    case kSpace: return "The size and kind of room, from a small ambience through rooms, chambers and halls to vast cathedrals. It is stepless: turn it and the space morphs smoothly, with a matching decay time.";
    case kDecayRate: return "Scales the decay time of the chosen space from 50% to 200%. The display shows the resulting decay time on its right-hand scale.";
    case kBrightness: return "The tone of the reverb: darker or brighter, and how quickly the high frequencies die away compared with the rest.";
    case kCharacter: return "From a clean, smooth, transparent decay (left), through a livelier sound with distinct reflections and echoes (middle), to an over-modulated, chorus-like tail (right).";
    case kDistance: return "Where the source sits in the room. Low values put it close, with strong early reflections; high values push it back, with softer reflections, more air and a later reverb.";
    case kWidth: return "Stereo width of the reverb: 0% is mono, 100% is natural stereo, up to 120% is extra wide.";
    case kMix: return "Balance between the dry signal and the reverb. Use 100% when Geode is on an effects send.";
    case kMixLock: return "Keeps the MIX setting when you load presets, so you can browse presets on a send (100%) or an insert without the balance jumping.";
    case kOutput: return "Output level, after the mix.";
    case kPredelay: return "Delay before the reverb starts, 0-500 ms. A little predelay keeps the source clear of its reverb. With SYNC on, this knob sets a note value instead.";
    case kPredelaySync: return "Sets the predelay as a note value at the host tempo (1/64 to 1/2, straight, dotted and triplet), up to 500 ms.";
    case kPredelayNote: return "The predelay as a note value at the host tempo.";
    case kAnalyzer: return "Shows the spectrum of the reverb (after the post EQ) behind the curves.";
    case kMidiOn: return "Turns MIDI control on or off. When off, learned controllers are ignored.";
    default: break;
    }
    if (p >= kDecayFirst && p <= kDecayLast) {
        switch (((int)p - (int)kDecayFirst) % kDFields) {
        case kDOn: return "Switches this decay band in or out without deleting it. Alt-click its dot does the same.";
        case kDShape: return "BELL changes the decay around a frequency, LOW and HIGH SHELF everything below or above it. NOTCH makes the decay very short in a narrow band, to remove a ringing frequency from the tail.";
        case kDFreq: return "Centre or corner frequency of the band. Drag the dot left and right.";
        case kDAmount: return "How much longer (up) or shorter (down) the reverb decays in this band: from a quarter to four times as long. Drag the dot up and down.";
        case kDQ: return "Width of the band: high Q is narrow, low Q is broad. Mouse wheel over a dot changes it.";
        default: return "";
        }
    }
    switch (((int)p - (int)kPostFirst) % kPFields) {
    case kPOn: return "Switches this post EQ band in or out without deleting it. Alt-click its dot does the same.";
    case kPShape: return "BELL boosts or cuts around a frequency, LOW and HIGH SHELF everything below or above it, LOW CUT and HIGH CUT remove everything below or above it.";
    case kPFreq: return "Centre or corner frequency. Drag the dot left and right.";
    case kPGain: return "Boost or cut in dB. Drag the dot up and down. Cut filters have no gain; their dot sets the resonance instead.";
    case kPQ: return "Bandwidth: high Q is narrow, low Q is broad. For cuts it adds a resonant peak. Mouse wheel over a dot changes it.";
    case kPSlope: return "Steepness of a cut filter: 6 to 96 dB per octave.";
    default: return "";
    }
}

enum TipTarget { kTipUndo, kTipRedo, kTipLearn, kTipMidi, kTipHelp, kTipA, kTipB, kTipCopy, kTipPrev, kTipPresets, kTipNext,
                 kTipDot, kTipDisplay, kTipDelete, kTipLayerDecay, kTipLayerPost, kTipAnalyzer, kTipMeters, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "MIDI", "Help", "Setting A", "Setting B", "Copy A/B",
                                                   "Previous preset", "Presets", "Next preset", "EQ band", "Reverb display", "Delete band",
                                                   "Decay rate EQ", "Post EQ", "Analyzer", "Meters" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including new and deleted bands, preset loads and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Band knobs learn the band that is selected. Right-click a control to unassign.";
    case kTipMidi: return paramTip(kMidiOn);
    case kTipHelp: return "Opens the help pages: the space controls, the two EQs, predelay and mix, MIDI and shortcuts.";
    case kTipA: case kTipB: return "Geode keeps two complete settings, A and B. Click the other letter to switch and compare.";
    case kTipCopy: return "Copies the current setting to the other slot.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser. A * after the name means you have changed it.";
    case kTipNext: return "Load the next preset.";
    case kTipDot: return "Drag to move: left/right = frequency, up/down = decay change (decay EQ) or gain (post EQ). Mouse wheel = Q. Alt-click switches the band on/off, right-click (or Delete) removes it.";
    case kTipDisplay: return "The golden line is the decay time at each frequency (right-hand scale), the violet line is the post EQ (left-hand scale), behind them the spectrum of the reverb. Double-click, or click on the active curve, to add a band to the active EQ.";
    case kTipDelete: return "Removes the selected band. Undo brings it back.";
    case kTipLayerDecay: return "Edit the decay rate EQ: up to six bands that make the reverb decay longer or shorter at some frequencies, without changing its level.";
    case kTipLayerPost: return "Edit the post EQ: up to six bands that shape the sound of the reverb itself, with cuts up to 96 dB/oct.";
    case kTipAnalyzer: return paramTip(kAnalyzer);
    case kTipMeters: return "Input and output levels.";
    default: return "";
    }
}

struct HelpPage { const char* title; const char* body; };
static const HelpPage kHelpPages[] = {
    { "Overview",
      "Geode is an algorithmic reverb with musical controls.\n\n"
      "1. Pick a SPACE: turn the knob from small ambiences to huge cathedrals. The decay time follows.\n"
      "2. Adjust DECAY RATE (50-200%), BRIGHTNESS and CHARACTER to taste.\n"
      "3. Place the source with DISTANCE, and set the STEREO WIDTH.\n"
      "4. Set MIX (100% on an effects send), and PREDELAY to keep the source clear.\n"
      "5. Shape the tail with the DECAY RATE EQ and the POST EQ in the display.\n\n"
      "Hover over anything for a moment to see what it does." },
    { "The space",
      "SPACE fades steplessly between fourteen room models, from Small Ambience to Vast Cathedral, without clicks.\n\n"
      "DECAY RATE scales the decay time of the space from 50% to 200%.\n\n"
      "BRIGHTNESS sets the tone, and how fast the highs die away.\n\n"
      "CHARACTER goes from a clean, transparent decay, through a lively sound with distinct reflections and echoes, to an over-modulated, chorus-like effect.\n\n"
      "DISTANCE moves the source from close (strong early reflections) to far (soft reflections, more air, a later tail).\n\n"
      "STEREO WIDTH goes from mono (0%) through stereo (100%) to extra wide (120%)." },
    { "Decay rate EQ",
      "The golden curve shows how long the reverb lasts at each frequency, on the right-hand scale in seconds.\n\n"
      "Click DECAY RATE EQ, then double-click the display to add a band. Drag a dot up to make the reverb ring longer there, down to make it die away sooner: from a quarter to four times the decay time. The level of the reverb doesn't change, only how long it lasts.\n\n"
      "Shapes: bell, low shelf, high shelf, and notch (a very short decay in a narrow band, for removing a ringing frequency). Up to six bands." },
    { "Post EQ",
      "The violet curve is an EQ on the reverb only (the dry signal is untouched), on the left-hand scale in dB.\n\n"
      "Click POST EQ, then double-click the display to add a band. Shapes: bell, low and high shelf, and low and high cut filters with slopes from 6 to 96 dB per octave. Up to six bands.\n\n"
      "A low cut around 150-300 Hz keeps the reverb from muddying the mix." },
    { "Mix & predelay",
      "MIX balances dry and reverb. On an effects send, use 100%. LOCK keeps the mix when you load presets.\n\n"
      "PREDELAY (0-500 ms) delays the start of the reverb. With SYNC, it is a note value at the host tempo (1/64 to 1/2, straight, dotted or triplet).\n\n"
      "OUTPUT sets the final level." },
    { "MIDI & shortcuts",
      "MIDI LEARN: click LEARN, click a control, move a controller. Right-click unassigns.\n\n"
      "A/B holds two complete settings; COPY copies the current one to the other slot. Every edit is an undo step (Cmd/Ctrl-Z, Cmd/Ctrl-Shift-Z).\n\n"
      "Knobs: drag, Shift-drag for fine control, double-click to type a value, Cmd/Ctrl-click to reset.\n"
      "EQ dots: drag; mouse wheel = Q; Alt-click = on/off; right-click or Delete = remove.\n"
      "Esc closes this window." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace geode
