// Parameter metadata shared by the DSP wrapper and the UI. License: MIT.
#pragma once
#include "GeodeEngine.hpp"
#include <cstdio>

namespace geode {

static constexpr uint32_t kHintInteger = 1, kHintBoolean = 2, kHintLog = 4, kHintOutput = 8, kHintNoAuto = 16;
struct ParamDef { const char* name; const char* sym; const char* unit; float min, max; uint32_t hints; };

inline const ParamDef& paramDef(uint32_t i) {
    static ParamDef t[kParamCount];
    static bool done = false;
    if (!done) {
        done = true;
        auto set = [](uint32_t i, const char* n, const char* s, const char* u, float lo, float hi, uint32_t h) { t[i] = { n, s, u, lo, hi, h }; };
        const uint32_t I = kHintInteger, B = kHintBoolean | kHintInteger, O = kHintOutput, L = kHintLog;
        set(kSpace,        "Space",            "space",     "",   0, 1, 0);
        set(kDecayRate,    "Decay Rate",       "decay",     "%",  50, 200, 0);
        set(kBrightness,   "Brightness",       "bright",    "%",  0, 100, 0);
        set(kCharacter,    "Character",        "character", "%",  0, 100, 0);
        set(kDistance,     "Distance",         "distance",  "%",  0, 100, 0);
        set(kWidth,        "Stereo Width",     "width",     "%",  0, 120, 0);
        set(kMix,          "Mix",              "mix",       "%",  0, 100, 0);
        set(kMixLock,      "Mix Lock",         "mixlock",   "",   0, 1, B | kHintNoAuto);
        set(kOutput,       "Output Level",     "output",    "dB", -36, 12, 0);
        set(kPredelay,     "Predelay",         "predelay",  "ms", 0, 500, 0);
        set(kPredelaySync, "Predelay Sync",    "pdsync",    "",   0, 1, B);
        set(kPredelayNote, "Predelay Note",    "pdnote",    "",   0, kNumNotes - 1, I);
        set(kAnalyzer,     "Analyzer",         "analyzer",  "",   0, 1, B | kHintNoAuto);
        set(kMidiOn,       "MIDI",             "midion",    "",   0, 1, B);
        static char dn[kDecayBands][kDFields][32], ds[kDecayBands][kDFields][16], pn[kPostBands][kPFields][32], ps[kPostBands][kPFields][16];
        static const char* dfn[kDFields] = { "Used", "Enabled", "Shape", "Frequency", "Decay", "Q" };
        static const char* dfs[kDFields] = { "used", "on", "shape", "freq", "amt", "q" };
        for (int b = 0; b < kDecayBands; ++b)
            for (int f = 0; f < kDFields; ++f) {
                std::snprintf(dn[b][f], 32, "Decay EQ %d %s", b + 1, dfn[f]); std::snprintf(ds[b][f], 16, "d%d%s", b + 1, dfs[f]);
                const uint32_t i = dp(b, f);
                switch (f) {
                case kDUsed: case kDOn: set(i, dn[b][f], ds[b][f], "", 0, 1, B); break;
                case kDShape: set(i, dn[b][f], ds[b][f], "", 0, kNumDShapes - 1, I); break;
                case kDFreq: set(i, dn[b][f], ds[b][f], "Hz", 20, 20000, L); break;
                case kDAmount: set(i, dn[b][f], ds[b][f], "oct", -2, 2, 0); break;
                default: set(i, dn[b][f], ds[b][f], "", 0.1f, 10, L); break;
                }
            }
        static const char* pfn[kPFields] = { "Used", "Enabled", "Shape", "Frequency", "Gain", "Q", "Slope" };
        static const char* pfs[kPFields] = { "used", "on", "shape", "freq", "gain", "q", "slope" };
        for (int b = 0; b < kPostBands; ++b)
            for (int f = 0; f < kPFields; ++f) {
                std::snprintf(pn[b][f], 32, "Post EQ %d %s", b + 1, pfn[f]); std::snprintf(ps[b][f], 16, "p%d%s", b + 1, pfs[f]);
                const uint32_t i = pp(b, f);
                switch (f) {
                case kPUsed: case kPOn: set(i, pn[b][f], ps[b][f], "", 0, 1, B); break;
                case kPShape: set(i, pn[b][f], ps[b][f], "", 0, kNumPShapes - 1, I); break;
                case kPFreq: set(i, pn[b][f], ps[b][f], "Hz", 10, 30000, L); break;
                case kPGain: set(i, pn[b][f], ps[b][f], "dB", -30, 30, 0); break;
                case kPQ: set(i, pn[b][f], ps[b][f], "", 0.025f, 40, L); break;
                default: set(i, pn[b][f], ps[b][f], "", 0, 4, I); break;
                }
            }
        set(kOutLearn, "MIDI Learn",     "o_learn", "",   0, 10000000, O);
        set(kOutInL,   "Input Level L",  "o_inl",   "dB", -120, 24, O);
        set(kOutInR,   "Input Level R",  "o_inr",   "dB", -120, 24, O);
        set(kOutOutL,  "Output Level L", "o_outl",  "dB", -120, 24, O);
        set(kOutOutR,  "Output Level R", "o_outr",  "dB", -120, 24, O);
    }
    return t[i];
}

inline int decayBandOf(uint32_t p) { return (p >= kDecayFirst && p <= kDecayLast) ? (int)((p - kDecayFirst) / kDFields) : -1; }
inline int decayFieldOf(uint32_t p) { return (p >= kDecayFirst && p <= kDecayLast) ? (int)((p - kDecayFirst) % kDFields) : -1; }
inline int postBandOf(uint32_t p) { return (p >= kPostFirst && p <= kPostLast) ? (int)((p - kPostFirst) / kPFields) : -1; }
inline int postFieldOf(uint32_t p) { return (p >= kPostFirst && p <= kPostLast) ? (int)((p - kPostFirst) % kPFields) : -1; }

inline const char* const* enumLabels(uint32_t p, int& count) {
    static const char* const notes[kNumNotes] = { "1/64", "1/32 T", "1/32", "1/16 T", "1/16", "1/16 D", "1/8 T", "1/8", "1/8 D", "1/4 T", "1/4", "1/4 D", "1/2" };
    static const char* const dshape[] = { "Bell", "Low Shelf", "High Shelf", "Notch" };
    static const char* const pshape[] = { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut" };
    static const char* const slope[] = { "6 dB/oct", "12 dB/oct", "24 dB/oct", "48 dB/oct", "96 dB/oct" };
    if (p == kPredelayNote) { count = kNumNotes; return notes; }
    if (decayFieldOf(p) == kDShape) { count = kNumDShapes; return dshape; }
    if (postFieldOf(p) == kPShape) { count = kNumPShapes; return pshape; }
    if (postFieldOf(p) == kPSlope) { count = 5; return slope; }
    count = 0; return nullptr;
}

inline float toNorm(uint32_t p, float v) {
    const ParamDef& d = paramDef(p);
    if (d.hints & kHintLog) return std::log(clampf(v, d.min, d.max) / d.min) / std::log(d.max / d.min);
    return (clampf(v, d.min, d.max) - d.min) / (d.max - d.min);
}
inline float fromNorm(uint32_t p, float n) {
    const ParamDef& d = paramDef(p);
    n = clampf(n, 0.f, 1.f);
    float v = (d.hints & kHintLog) ? d.min * std::pow(d.max / d.min, n) : d.min + n * (d.max - d.min);
    if (d.hints & kHintInteger) v = std::round(v);
    return v;
}
inline bool isViewParam(uint32_t p) { return (paramDef(p).hints & kHintNoAuto) != 0; }

} // namespace geode
