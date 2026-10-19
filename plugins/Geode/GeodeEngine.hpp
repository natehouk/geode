// Geode — algorithmic reverb engine.
// Signal path: predelay (with air absorption for distance) -> early reflections + input diffusion ->
// 16-line feedback delay network (Hadamard mixing, modulated cubic reads) whose per-line loss filters realise a
// frequency-dependent decay time (the space's natural damping, brightness and the 6-band decay rate EQ) ->
// stereo width -> tone -> 6-band post EQ -> dry/wet mix.
// Clean-room implementation from public, generic techniques (Jot/Chaigne FDN with absorptive loss filters,
// Schroeder allpass diffusion, RBJ biquads, Zavalishin/Simper trapezoidal SVF). No third-party product code,
// assets or disassembly were used. License: MIT (see LICENSE).
#pragma once
#include "Fft.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace geode {

static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float dbToGain(float db) { return std::pow(10.f, db / 20.f); }

static constexpr int kDecayBands = 6, kPostBands = 6;
enum DField { kDUsed, kDOn, kDShape, kDFreq, kDAmount, kDQ, kDFields };
enum PField { kPUsed, kPOn, kPShape, kPFreq, kPGain, kPQ, kPSlope, kPFields };

// ------------------------------------------------------------------ params (append-only)
enum Param : uint32_t {
    kSpace, kDecayRate, kBrightness, kCharacter, kDistance, kWidth, kMix, kMixLock, kOutput,
    kPredelay, kPredelaySync, kPredelayNote, kAnalyzer, kMidiOn,
    kDecayFirst,
    kDecayLast = kDecayFirst + kDecayBands * kDFields - 1,
    kPostFirst,
    kPostLast = kPostFirst + kPostBands * kPFields - 1,
    kOutLearn, kOutInL, kOutInR, kOutOutL, kOutOutR,
    kParamCount
};
inline uint32_t dp(int b, int f) { return kDecayFirst + b * kDFields + f; }
inline uint32_t pp(int b, int f) { return kPostFirst + b * kPFields + f; }
enum DShape { kDBell, kDLowShelf, kDHighShelf, kDNotch, kNumDShapes };
enum PShape { kPBell, kPLowShelf, kPHighShelf, kPLowCut, kPHighCut, kNumPShapes };
static constexpr float kNotchL = -2.5f;                // a decay notch shortens the decay about 6 times

// ------------------------------------------------------------------ spaces
struct SpaceModel { const char* name; float sizeMs, decayS, erMs, hf, bass; };
// sizeMs: longest feedback delay; decayS: natural RT60 at mid frequencies; erMs: early reflection window;
// hf / bass: natural decay change above 4 kHz / below 250 Hz, in octaves of decay time
static const SpaceModel kSpaces[] = {
    { "Small Ambience",  9.f, 0.30f,  8.f, -0.5f, -0.2f },
    { "Small Room",     14.f, 0.45f, 12.f, -0.6f, -0.1f },
    { "Vocal Booth",    18.f, 0.55f, 14.f, -0.9f, -0.2f },
    { "Studio Room",    24.f, 0.75f, 20.f, -0.7f,  0.0f },
    { "Medium Room",    32.f, 0.95f, 26.f, -0.7f,  0.1f },
    { "Large Room",     42.f, 1.25f, 34.f, -0.8f,  0.1f },
    { "Chamber",        50.f, 1.55f, 38.f, -0.6f,  0.0f },
    { "Small Hall",     62.f, 1.85f, 46.f, -0.8f,  0.2f },
    { "Concert Hall",   78.f, 2.35f, 56.f, -0.9f,  0.3f },
    { "Large Hall",     94.f, 3.00f, 66.f, -1.0f,  0.3f },
    { "Church",        108.f, 3.80f, 74.f, -1.1f,  0.4f },
    { "Large Church",  122.f, 4.80f, 82.f, -1.2f,  0.4f },
    { "Cathedral",     138.f, 6.20f, 90.f, -1.3f,  0.5f },
    { "Vast Cathedral",152.f, 8.00f, 98.f, -1.4f,  0.5f },
};
static constexpr int kNumSpaces = sizeof(kSpaces) / sizeof(kSpaces[0]);
struct SpaceParams { float sizeMs, decayS, erMs, hf, bass; };
inline SpaceParams spaceAt(float s) {                  // stepless: interpolate between neighbouring models
    const float x = clampf(s, 0.f, 1.f) * (kNumSpaces - 1);
    const int i = std::min(kNumSpaces - 2, (int)x); const float t = x - i;
    const SpaceModel &a = kSpaces[i], &b = kSpaces[i + 1];
    auto lg = [&](float u, float v) { return std::exp(std::log(u) + (std::log(v) - std::log(u)) * t); };
    return { lg(a.sizeMs, b.sizeMs), lg(a.decayS, b.decayS), lg(a.erMs, b.erMs), a.hf + (b.hf - a.hf) * t, a.bass + (b.bass - a.bass) * t };
}
inline int spaceIndex(float s) { return (int)std::round(clampf(s, 0.f, 1.f) * (kNumSpaces - 1)); }

// predelay note values, in quarter notes
static constexpr int kNumNotes = 13;
inline float noteBeats(int n) {
    static const float t[kNumNotes] = { 1 / 16.f, 1 / 12.f, 1 / 8.f, 1 / 6.f, 1 / 4.f, 3 / 8.f, 1 / 3.f, 1 / 2.f, 3 / 4.f, 2 / 3.f, 1.f, 1.5f, 2.f };
    return t[n < 0 ? 0 : (n >= kNumNotes ? kNumNotes - 1 : n)];
}

// ------------------------------------------------------------------ biquads (decay loss filters, tone)
struct BQ { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; bool identity = true; };   // an identity filter keeps a zero state, so it can be skipped
enum { kRbjPeak, kRbjLowShelf, kRbjHighShelf };
inline BQ rbj(int type, float fs, float f, float db, float q) {
    BQ r; if (std::fabs(db) < 1e-6f) return r;
    const double A = std::pow(10.0, db / 40.0), w = 2.0 * 3.14159265358979 * clampf(f, 10.f, 0.45f * fs) / fs;
    const double cs = std::cos(w), sn = std::sin(w), al = sn / (2.0 * std::max(0.05f, q)), sa = 2.0 * std::sqrt(A) * al;
    double b0, b1, b2, a0, a1, a2;
    if (type == kRbjPeak) { b0 = 1 + al * A; b1 = -2 * cs; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * cs; a2 = 1 - al / A; }
    else if (type == kRbjLowShelf) {
        b0 = A * ((A + 1) - (A - 1) * cs + sa); b1 = 2 * A * ((A - 1) - (A + 1) * cs); b2 = A * ((A + 1) - (A - 1) * cs - sa);
        a0 = (A + 1) + (A - 1) * cs + sa; a1 = -2 * ((A - 1) + (A + 1) * cs); a2 = (A + 1) + (A - 1) * cs - sa;
    } else {
        b0 = A * ((A + 1) + (A - 1) * cs + sa); b1 = -2 * A * ((A - 1) + (A + 1) * cs); b2 = A * ((A + 1) + (A - 1) * cs - sa);
        a0 = (A + 1) - (A - 1) * cs + sa; a1 = 2 * ((A - 1) - (A + 1) * cs); a2 = (A + 1) - (A - 1) * cs - sa;
    }
    r.identity = false;
    r.b0 = (float)(b0 / a0); r.b1 = (float)(b1 / a0); r.b2 = (float)(b2 / a0); r.a1 = (float)(a1 / a0); r.a2 = (float)(a2 / a0);
    return r;
}
inline float bqDb(const BQ& q, float w) {
    const cpx z1 = std::polar(1.f, -w), z2 = z1 * z1;
    const cpx h = (q.b0 + q.b1 * z1 + q.b2 * z2) / (1.f + q.a1 * z1 + q.a2 * z2);
    return 20.f * std::log10(std::max(std::abs(h), 1e-9f));
}

// The decay shape: a list of (filter type, frequency, decay change in octaves, Q). Each becomes one biquad per delay
// line whose dB gain is scaled to that line's length, so every line loses the same dB per second at each frequency.
struct DecayShape { int n = 0; int type[10]; float f[10], L[10], q[10]; };
// fixed slots (2 space shelves + 6 bands), so a band switching on or off never moves another filter's state
inline float bandDecayL(const float* p, int b) {
    if (p[dp(b, kDUsed)] < 0.5f || p[dp(b, kDOn)] < 0.5f) return 0.f;
    return (int)(p[dp(b, kDShape)] + 0.5f) == kDNotch ? kNotchL : clampf(p[dp(b, kDAmount)], -2.f, 2.f);
}
inline DecayShape decayShape(const float* p, const SpaceParams& sp, float bright, const float* effL = nullptr) {
    DecayShape s;
    auto add = [&](int t, float f, float L, float q) { s.type[s.n] = t; s.f[s.n] = f; s.L[s.n] = L; s.q[s.n] = q; ++s.n; };
    add(kRbjLowShelf, 250.f, sp.bass, 0.6f);
    add(kRbjHighShelf, 4000.f, std::min(0.4f, sp.hf + (bright - 0.5f) * 2.4f), 0.6f);
    for (int b = 0; b < kDecayBands; ++b) {
        const int sh = (int)(p[dp(b, kDShape)] + 0.5f);
        add(sh == kDLowShelf ? kRbjLowShelf : (sh == kDHighShelf ? kRbjHighShelf : kRbjPeak), clampf(p[dp(b, kDFreq)], 20.f, 20000.f),
            effL ? effL[b] : bandDecayL(p, b), clampf(p[dp(b, kDQ)], 0.1f, 10.f));
    }
    return s;
}
static constexpr float kMaxDecayS = 40.f;
struct LineFilter { BQ bq[10]; int n = 0; float g = 1; };
// The bands' decay changes multiply (they add in octaves of decay time), but the loss filters' dB gains add.
// So the band gains are solved for on a reference line: a few correction passes match the realised loss at each band's
// own frequency to the target loss there. Other lines scale the solved dB gains by their length.
struct DecaySolution { int n = 0; int type[10]; float f[10], q[10], gRef[10]; float dRef = 1, aRef = -1; };
inline float evalHz(const DecayShape& s, int i, float fs) {
    const float f = s.type[i] == kRbjLowShelf ? s.f[i] / 3.f : (s.type[i] == kRbjHighShelf ? s.f[i] * 3.f : s.f[i]);
    return clampf(f, 15.f, 0.42f * fs);
}
inline DecaySolution solveDecay(const DecayShape& s, float T0, float fs, float dRef) {
    DecaySolution r; r.n = s.n; r.dRef = dRef; r.aRef = -60.f * dRef / (fs * T0);
    BQ unit[10], cur[10]; float target[10], w[10];
    for (int i = 0; i < s.n; ++i) { r.type[i] = s.type[i]; r.f[i] = s.f[i]; r.q[i] = s.q[i]; unit[i] = rbj(s.type[i], fs, s.f[i], 6.f, s.q[i]); }
    for (int j = 0; j < s.n; ++j) {
        w[j] = 2.f * kPi * evalHz(s, j, fs) / fs;
        float L = 0; for (int i = 0; i < s.n; ++i) L += s.L[i] * bqDb(unit[i], w[j]) / 6.f;
        target[j] = r.aRef * std::exp2(-L);
    }
    for (int i = 0; i < s.n; ++i) { r.gRef[i] = r.aRef * (std::exp2(-s.L[i]) - 1.f); cur[i] = rbj(s.type[i], fs, s.f[i], r.gRef[i], s.q[i]); }
    for (int it = 0; it < 5; ++it)
        for (int j = 0; j < s.n; ++j) {
            if (std::fabs(s.L[j]) < 1e-4f) continue;            // empty slots stay flat
            float real = r.aRef; for (int i = 0; i < s.n; ++i) real += bqDb(cur[i], w[j]);
            r.gRef[j] = clampf(r.gRef[j] + 0.9f * (target[j] - real), -200.f, 200.f);
            cur[j] = rbj(s.type[j], fs, s.f[j], r.gRef[j], s.q[j]);
        }
    return r;
}
// loss filter for a line of d samples
inline void designLine(const DecaySolution& r, float fs, float d, LineFilter& lf, float extraDb = 0) {
    const float k = d / r.dRef;
    lf.n = r.n; lf.g = dbToGain(r.aRef * k + extraDb * d);
    for (int i = 0; i < r.n; ++i) lf.bq[i] = rbj(r.type[i], fs, r.f[i], r.gRef[i] * k, r.q[i]);
}
inline float lineDb(const LineFilter& lf, float fs, float hz) {
    const float w = 2.f * kPi * hz / fs; float db = 20.f * std::log10(std::max(lf.g, 1e-9f));
    for (int i = 0; i < lf.n; ++i) db += bqDb(lf.bq[i], w);
    return db;
}
// keeps the decay below kMaxDecayS everywhere: returns a dB-per-sample correction (<= 0)
inline float decayCorrection(const DecaySolution& r, float fs) {
    LineFilter lf; designLine(r, fs, r.dRef, lf);
    const float allowed = -60.f * r.dRef / (fs * kMaxDecayS);
    float worst = -1e9f;
    for (int k = 0; k < 40; ++k) { const float hz = 20.f * std::pow(0.45f * fs / 20.f, k / 39.f); worst = std::max(worst, lineDb(lf, fs, hz)); }
    return worst > allowed ? (allowed - worst) / r.dRef : 0.f;
}
inline float decayTime(const float* p) { return spaceAt(p[kSpace]).decayS * clampf(p[kDecayRate], 50.f, 200.f) / 100.f; }
// the realised decay time (RT60, seconds) at hz, for the display
struct DecayCurve {
    LineFilter lf; float d = 1, fs = 48000;
    void build(const float* p, float sr) {
        fs = sr; const SpaceParams sp = spaceAt(p[kSpace]); const float T0 = decayTime(p);
        d = sp.sizeMs * 0.001f * fs * 0.65f;
        const DecaySolution r = solveDecay(decayShape(p, sp, clampf(p[kBrightness] / 100.f, 0.f, 1.f)), T0, fs, d);
        designLine(r, fs, d, lf, decayCorrection(r, fs));
    }
    float timeAt(float hz) const { const float db = lineDb(lf, fs, std::min(hz, 0.45f * fs)); return db < -1e-6f ? -60.f * d / (fs * db) : kMaxDecayS; }
};

// ------------------------------------------------------------------ post EQ band (TPT SVF, as in Mica)
inline int slopeDb(int s) { static const int t[5] = { 6, 12, 24, 48, 96 }; return t[s < 0 ? 0 : (s > 4 ? 4 : s)]; }
inline int cutStages(int s) { return s <= 1 ? 1 : (s == 2 ? 2 : (s == 3 ? 4 : 8)); }
inline float bwK(int stages, int i) { return 2.f * std::cos(kPi * (2 * i + 1) / (4.f * stages)); }   // Butterworth damping
inline cpx postResponse(int shape, int slope, float fc, float gainDb, float q, float hz) {
    const cpx s(0.f, hz / fc);
    const float A = std::pow(10.f, gainDb / 40.f), sq = std::sqrt(A);
    switch (shape) {
    case kPBell:      return (s * s + s * (A / q) + 1.f) / (s * s + s / (A * q) + 1.f);
    case kPLowShelf:  return A * (s * s + s * (sq / q) + A) / (A * s * s + s * (sq / q) + 1.f);
    case kPHighShelf: return A * (A * s * s + s * (sq / q) + 1.f) / (s * s + s * (sq / q) + A);
    default: {
        const bool hp = shape == kPLowCut;
        if (slope == 0) return hp ? s / (s + 1.f) : cpx(1.f, 0.f) / (s + 1.f);
        const int st = cutStages(slope); cpx h(1.f, 0.f);
        for (int i = 0; i < st; ++i) { const float k = i == st - 1 ? bwK(st, i) * 0.70710678f / q : bwK(st, i); h *= (hp ? s * s : cpx(1.f, 0.f)) / (s * s + k * s + 1.f); }
        return h;
    }
    }
}
struct PostBand {
    bool used = false, on = false; int shape = kPBell, slope = 1;
    int curShape = kPBell, curSlope = 1; bool swapping = false; int hold = 0;
    float oct = 10, gain = 0, lq = 0, mix = 0, m = 0, dm = 0;
    int nst = 1; bool onePole = false;
    float a1[8] = {}, a2[8] = {}, a3[8] = {}, m0[8] = {}, m1[8] = {}, m2[8] = {}, G = 0;
    float ic1[2][8] = {}, ic2[2][8] = {}, op[2] = {};
    void reset() { std::memset(ic1, 0, sizeof ic1); std::memset(ic2, 0, sizeof ic2); op[0] = op[1] = 0; }
    void design(float fs) {
        const float fc = clampf(std::exp2(oct), 5.f, 0.49f * fs), q = std::exp2(lq), A = std::pow(10.f, gain / 40.f), t = std::tan(kPi * fc / fs);
        onePole = false; nst = 1;
        auto setSvf = [&](int i, float g, float k, float c0, float c1, float c2) { a1[i] = 1.f / (1.f + g * (g + k)); a2[i] = g * a1[i]; a3[i] = g * a2[i]; m0[i] = c0; m1[i] = c1; m2[i] = c2; };
        switch (curShape) {
        case kPBell: { const float k = 1.f / (q * A); setSvf(0, t, k, 1.f, k * (A * A - 1.f), 0.f); break; }
        case kPLowShelf: { const float k = 1.f / q; setSvf(0, t / std::sqrt(A), k, 1.f, k * (A - 1.f), A * A - 1.f); break; }
        case kPHighShelf: { const float k = 1.f / q; setSvf(0, t * std::sqrt(A), k, A * A, k * (1.f - A) * A, 1.f - A * A); break; }
        default: {
            const bool hp = curShape == kPLowCut;
            if (curSlope == 0) { onePole = true; G = t / (1.f + t); break; }
            nst = cutStages(curSlope);
            for (int i = 0; i < nst; ++i) { const float k = i == nst - 1 ? bwK(nst, i) * 0.70710678f / q : bwK(nst, i);
                if (hp) setSvf(i, t, k, 1.f, -k, -1.f); else setSvf(i, t, k, 0.f, 0.f, 1.f); }
        }
        }
    }
    inline float tick(float x, int c) {
        if (onePole) { const float v = (x - op[c]) * G, lp = v + op[c]; op[c] = lp + v; return curShape == kPLowCut ? x - lp : lp; }
        for (int i = 0; i < nst; ++i) {
            const float v3 = x - ic2[c][i], v1 = a1[i] * ic1[c][i] + a2[i] * v3, v2 = ic2[c][i] + a2[i] * ic1[c][i] + a3[i] * v3;
            ic1[c][i] = 2.f * v1 - ic1[c][i]; ic2[c][i] = 2.f * v2 - ic2[c][i];
            x = m0[i] * x + m1[i] * v1 + m2[i] * v2;
        }
        return x;
    }
};

// delay line lengths relative to the space size (mutually incommensurate, spread over 0.3-1.0)
static constexpr int kLines = 16, kTaps = 12, kLongest = 11;
static const float kRatio[kLines] = { 0.3013f, 0.8571f, 0.4219f, 0.6977f, 0.3467f, 0.9311f, 0.5323f, 0.7643f,
                                      0.3881f, 0.6211f, 0.4729f, 1.0000f, 0.5717f, 0.8093f, 0.4483f, 0.6599f };
static const float kSgnL[kLines] = { 0.25f, -0.25f, 0.25f, 0.25f, -0.25f, 0.25f, -0.25f, -0.25f, 0.25f, 0.25f, -0.25f, 0.25f, 0.25f, -0.25f, -0.25f, 0.25f };
static const float kSgnR[kLines] = { 0.25f, 0.25f, -0.25f, 0.25f, 0.25f, -0.25f, -0.25f, 0.25f, -0.25f, 0.25f, 0.25f, 0.25f, -0.25f, -0.25f, 0.25f, -0.25f };
static const float kSgnIn[kLines] = { 1, 1, -1, 1, 1, -1, 1, -1, -1, 1, 1, 1, -1, -1, 1, -1 };
static const float kErPos[2][kTaps] = { { 0.00f, 0.07f, 0.13f, 0.21f, 0.27f, 0.36f, 0.44f, 0.53f, 0.61f, 0.72f, 0.84f, 0.97f },
                                        { 0.03f, 0.09f, 0.17f, 0.23f, 0.31f, 0.39f, 0.48f, 0.57f, 0.66f, 0.77f, 0.89f, 1.00f } };
static const float kErSign[2][kTaps] = { { 0.9f, -0.7f, 0.8f, 0.6f, -0.7f, 0.5f, -0.6f, 0.5f, 0.45f, -0.4f, 0.35f, -0.3f },
                                         { 0.85f, 0.75f, -0.7f, 0.65f, 0.6f, -0.55f, 0.5f, -0.5f, 0.4f, 0.4f, -0.35f, 0.3f } };

// ------------------------------------------------------------------ engine
class Engine {
public:
    float p[kParamCount];

    Engine() {
        std::fill(p, p + kParamCount, 0.f);
        p[kSpace] = 0.5f; p[kDecayRate] = 100; p[kBrightness] = 50; p[kCharacter] = 25; p[kDistance] = 40; p[kWidth] = 100;
        p[kMix] = 30; p[kMixLock] = 0; p[kOutput] = 0; p[kPredelay] = 0; p[kPredelaySync] = 0; p[kPredelayNote] = 4; p[kAnalyzer] = 1; p[kMidiOn] = 1;
        for (int b = 0; b < kDecayBands; ++b) defaultDecayBand(b, 1000.f);
        for (int b = 0; b < kPostBands; ++b) defaultPostBand(b, 1000.f);
        p[kOutInL] = p[kOutInR] = p[kOutOutL] = p[kOutOutR] = -120;
        std::fill(ana, ana + kAnaSize, 0.f);
        setSampleRate(48000.f);
    }
    void defaultDecayBand(int b, float hz) { p[dp(b, kDUsed)] = 0; p[dp(b, kDOn)] = 1; p[dp(b, kDShape)] = kDBell; p[dp(b, kDFreq)] = hz; p[dp(b, kDAmount)] = 0; p[dp(b, kDQ)] = 1; }
    void defaultPostBand(int b, float hz) { p[pp(b, kPUsed)] = 0; p[pp(b, kPOn)] = 1; p[pp(b, kPShape)] = kPBell; p[pp(b, kPFreq)] = hz; p[pp(b, kPGain)] = 0; p[pp(b, kPQ)] = 1; p[pp(b, kPSlope)] = 2; }

    void setSampleRate(float sr) {
        fs = std::max(8000.f, sr);
        preLen = pow2((int)(0.75f * fs)); for (auto& v : pre) v.assign(preLen, 0.f);
        lineLen = pow2((int)(0.175f * fs)); for (auto& v : line) v.assign(lineLen, 0.f);
        apLen = pow2((int)(0.02f * fs)); for (auto& c : ap) for (auto& v : c) v.assign(apLen, 0.f);
        reset();
    }
    void setTempo(double bpm) { tempo = bpm > 1 ? (float)bpm : 120.f; }
    void reset() {
        for (auto& v : pre) std::fill(v.begin(), v.end(), 0.f);
        for (auto& v : line) std::fill(v.begin(), v.end(), 0.f);
        for (auto& c : ap) for (auto& v : c) std::fill(v.begin(), v.end(), 0.f);
        prePos = linePos = apPos = 0; air[0] = air[1] = 0; toneZ[0][0] = toneZ[0][1] = toneZ[1][0] = toneZ[1][1] = 0;
        std::memset(fz, 0, sizeof fz);
        snapTargets(); sm = tg; designAll(); lastDesign = sm;
        pdT = pdS = sm.pd * 0.001f * fs;
        std::copy(dS, dS + kLines, dCur); std::memcpy(erS, erT, sizeof erS); lateS = lateT;
        gDry = tDry = std::cos(sm.mix * kPi * 0.5f); gWet = tWet = std::sin(sm.mix * kPi * 0.5f); gOut = tOut = dbToGain(sm.out);
        for (int b = 0; b < kPostBands; ++b) { PostBand& d = post[b]; d.reset(); syncPost(b); d.curShape = d.shape; d.curSlope = d.slope;
            d.oct = std::log2(clampf(p[pp(b, kPFreq)], 10.f, 30000.f)); d.gain = p[pp(b, kPGain)]; d.lq = std::log2(clampf(p[pp(b, kPQ)], 0.025f, 40.f));
            d.mix = d.m = (d.used && d.on) ? 1.f : 0.f; d.dm = 0; d.design(fs); }
        for (int i = 0; i < kLines; ++i) lfo[i] = 0.37f * i;
        ctl = 0; mIn[0] = mIn[1] = mOut[0] = mOut[1] = 0;
    }
    float erTrim = 1, lateTrim = 1;     // test hooks: scale the early and late parts
    uint32_t latency() const { return 0; }
    float sampleRate() const { return fs; }
    float predelayMs() const {
        if (p[kPredelaySync] > 0.5f) return std::min(500.f, noteBeats((int)(p[kPredelayNote] + 0.5f)) * 60000.f / tempo);
        return clampf(p[kPredelay], 0.f, 500.f);
    }
    float meterIn(int c) const { return 20.f * std::log10(std::max(mIn[c], 1e-6f)); }
    float meterOut(int c) const { return 20.f * std::log10(std::max(mOut[c], 1e-6f)); }

    // analyser: mono wet signal after the post EQ
    static constexpr int kAnaSize = 16384;
    float ana[kAnaSize];
    struct AtomicIndex {
        std::atomic<uint32_t> v { 0 };
        AtomicIndex() {}
        AtomicIndex(const AtomicIndex& o) : v(o.v.load()) {}
        AtomicIndex& operator=(const AtomicIndex& o) { v.store(o.v.load()); return *this; }
        uint32_t load(std::memory_order m = std::memory_order_seq_cst) const { return v.load(m); }
        void store(uint32_t x, std::memory_order m = std::memory_order_seq_cst) { v.store(x, m); }
    } anaWrite;

    void process(const float* inL, const float* inR, float* outL, float* outR, uint32_t n) {
        uint32_t w = anaWrite.load(std::memory_order_relaxed);
        const float meterFall = std::exp(-1.f / (0.3f * fs));
        for (uint32_t i = 0; i < n; ++i) {
            if (--ctl <= 0) { ctl = kCtl; control(); }
            const float dl = inL[i], dr = inR[i];
            mIn[0] = std::max(mIn[0] * meterFall, std::fabs(dl)); mIn[1] = std::max(mIn[1] * meterFall, std::fabs(dr));
            // air absorption (distance), into the predelay line
            air[0] += (dl - air[0]) * airK; air[1] += (dr - air[1]) * airK;
            pre[0][prePos] = std::isfinite(air[0]) ? air[0] : 0.f; pre[1][prePos] = std::isfinite(air[1]) ? air[1] : 0.f;
            pdS += (pdT - pdS) * pdSmooth;
            // early reflections (tap times glide, so a changing space never jumps)
            float er[2] = { 0, 0 };
            for (int k = 0; k < kTaps; ++k) {
                erS[0][k] += (erT[0][k] - erS[0][k]) * kGlide; erS[1][k] += (erT[1][k] - erS[1][k]) * kGlide;
                er[0] += readPre(k & 1 ? 1 : 0, pdS + erS[0][k]) * erG[0][k];
                er[1] += readPre(k & 1 ? 0 : 1, pdS + erS[1][k]) * erG[1][k];
            }
            // late input: delayed, then diffused
            lateS += (lateT - lateS) * kGlide;
            float x[2] = { readPre(0, pdS + lateS), readPre(1, pdS + lateS) };
            for (int c = 0; c < 2; ++c) {
                x[c] += er[c] * 0.35f;
                for (int a = 0; a < kAp; ++a) {
                    std::vector<float>& b = ap[c][a];
                    const float d = b[(apPos - apD[c][a]) & (apLen - 1)], v = x[c] + diffG * d;
                    b[apPos] = v; x[c] = d - diffG * v;
                }
            }
            // feedback delay network
            float f[kLines];
            for (int l = 0; l < kLines; ++l) {
                lfo[l] += lfoInc[l]; if (lfo[l] > 1.f) lfo[l] -= 1.f;
                const float tri = 4.f * std::fabs(lfo[l] - 0.5f) - 1.f;           // triangle, smooth enough at these rates
                dCur[l] += (dS[l] - dCur[l]) * kGlide;
                float y = readLine(l, dCur[l] + modDepth * modScale[l] * tri);
                LineFilter& lf = lflt[l];
                y *= lf.g;
                for (int k = 0; k < lf.n; ++k) {                                    // transposed direct form II
                    const BQ& q = lf.bq[k]; float* z = fz[l][k];
                    if (q.identity) { z[0] = z[1] = 0.f; continue; }
                    const float o = q.b0 * y + z[0]; z[0] = q.b1 * y - q.a1 * o + z[1]; z[1] = q.b2 * y - q.a2 * o; y = o;
                }
                f[l] = y;
            }
            float wl = 0, wr = 0;
            for (int l = 0; l < kLines; ++l) { wl += f[l] * kSgnL[l]; wr += f[l] * kSgnR[l]; }
            hadamard(f);
            const float dith = (linePos & 1) ? 1e-18f : -1e-18f;
            for (int l = 0; l < kLines; ++l) {
                float v = f[l] + x[l & 1] * kSgnIn[l] * 0.5f + dith;
                if (!(std::fabs(v) < 1e4f)) v = 0.f;
                line[l][linePos] = v;
            }
            // wet: late + early, width, tone
            float wet[2] = { wl * lateGain + er[0] * erGain, wr * lateGain + er[1] * erGain };
            { const float m = 0.5f * (wet[0] + wet[1]), s = 0.5f * (wet[0] - wet[1]) * widthS; wet[0] = m + s; wet[1] = m - s; }
            for (int c = 0; c < 2; ++c) {
                const float o = tone.b0 * wet[c] + toneZ[c][0]; toneZ[c][0] = tone.b1 * wet[c] - tone.a1 * o + toneZ[c][1]; toneZ[c][1] = tone.b2 * wet[c] - tone.a2 * o; wet[c] = o;
            }
            for (PostBand& bd : post) {
                if (!bd.used) continue;
                bd.m += bd.dm;
                for (int c = 0; c < 2; ++c) { const float y = bd.tick(wet[c], c); if (bd.m > 0.f) wet[c] += (y - wet[c]) * bd.m; }
            }
            ana[w & (kAnaSize - 1)] = 0.5f * (wet[0] + wet[1]); ++w;
            gDry += (tDry - gDry) * 0.002f; gWet += (tWet - gWet) * 0.002f; gOut += (tOut - gOut) * 0.002f;
            float yl = (dl * gDry + wet[0] * gWet) * gOut, yr = (dr * gDry + wet[1] * gWet) * gOut;
            if (!(std::fabs(yl) < 64.f)) yl = std::isfinite(yl) ? std::copysign(64.f, yl) : 0.f;
            if (!(std::fabs(yr) < 64.f)) yr = std::isfinite(yr) ? std::copysign(64.f, yr) : 0.f;
            outL[i] = yl; outR[i] = yr;
            mOut[0] = std::max(mOut[0] * meterFall, std::fabs(yl)); mOut[1] = std::max(mOut[1] * meterFall, std::fabs(yr));
            prePos = (prePos + 1) & (preLen - 1); linePos = (linePos + 1) & (lineLen - 1); apPos = (apPos + 1) & (apLen - 1);
        }
        anaWrite.store(w, std::memory_order_release);
        // recover from anything non-finite in the loss filters or post EQ
        bool bad = false;
        for (int l = 0; l < kLines; ++l) for (int k = 0; k < 10; ++k) if (!(std::fabs(fz[l][k][0]) < 1e4f) || !(std::fabs(fz[l][k][1]) < 1e4f)) bad = true;
        if (bad) { std::memset(fz, 0, sizeof fz); for (auto& v : line) std::fill(v.begin(), v.end(), 0.f); }
        for (PostBand& bd : post) for (int c = 0; c < 2; ++c) for (int k = 0; k < 8; ++k) if (!(std::fabs(bd.ic1[c][k]) < 1e6f) || !(std::fabs(bd.ic2[c][k]) < 1e6f)) bd.reset();
        if (!(std::fabs(toneZ[0][0]) + std::fabs(toneZ[1][0]) < 1e6f)) std::memset(toneZ, 0, sizeof toneZ);
    }

    // combined post EQ response at hz (display)
    cpx postResponseAt(float hz) const {
        cpx h(1.f, 0.f);
        for (int b = 0; b < kPostBands; ++b) {
            if (p[pp(b, kPUsed)] < 0.5f || p[pp(b, kPOn)] < 0.5f) continue;
            h *= postResponse((int)(p[pp(b, kPShape)] + .5f), (int)(p[pp(b, kPSlope)] + .5f), p[pp(b, kPFreq)], p[pp(b, kPGain)], p[pp(b, kPQ)], hz);
        }
        return h;
    }

private:
    static constexpr int kCtl = 32, kAp = 4;
    static int pow2(int n) { int k = 1; while (k < n) k <<= 1; return k; }
    // smoothed control state
    struct CtlState {
        float space, decay, bright, chr, dist, width, mix, out, pd;
        float dec[kDecayBands * kDFields], effL[kDecayBands];
        bool operator==(const CtlState& o) const { return std::memcmp(this, &o, sizeof(CtlState)) == 0; }
    } tg, sm, lastDesign;
    void snapTargets() {
        tg.space = clampf(p[kSpace], 0.f, 1.f); tg.decay = clampf(p[kDecayRate], 50.f, 200.f); tg.bright = clampf(p[kBrightness], 0.f, 100.f) / 100.f;
        tg.chr = clampf(p[kCharacter], 0.f, 100.f) / 100.f; tg.dist = clampf(p[kDistance], 0.f, 100.f) / 100.f; tg.width = clampf(p[kWidth], 0.f, 120.f) / 100.f;
        tg.mix = clampf(p[kMix], 0.f, 100.f) / 100.f; tg.out = clampf(p[kOutput], -36.f, 12.f); tg.pd = predelayMs();
        for (int i = 0; i < kDecayBands * kDFields; ++i) tg.dec[i] = p[kDecayFirst + i];
        for (int b = 0; b < kDecayBands; ++b) tg.effL[b] = bandDecayL(p, b);
    }
    void control() {
        snapTargets();
        const float kFast = 1.f - std::exp(-kCtl / (0.03f * fs)), kSlow = 1.f - std::exp(-kCtl / (0.25f * fs));
        auto go = [](float& s, float t, float k) { s += (t - s) * k; if (std::fabs(t - s) < 1e-5f * (1.f + std::fabs(t))) s = t; };
        go(sm.space, tg.space, kSlow); go(sm.decay, tg.decay, kFast); go(sm.bright, tg.bright, kFast); go(sm.chr, tg.chr, kFast);
        go(sm.dist, tg.dist, kFast); go(sm.width, tg.width, kFast); go(sm.mix, tg.mix, kFast); go(sm.out, tg.out, kFast); sm.pd = tg.pd;
        for (int b = 0; b < kDecayBands; ++b)
            for (int f = 0; f < kDFields; ++f) {
                float& s = sm.dec[b * kDFields + f]; const float t = tg.dec[b * kDFields + f];
                if (f == kDFreq || f == kDQ) { const float ls = std::log(std::max(s, 1e-3f)), lt = std::log(std::max(t, 1e-3f)); float l = ls + (lt - ls) * kFast; if (std::fabs(lt - l) < 1e-4f) l = lt; s = std::exp(l); if (l == lt) s = t; }
                else if (f == kDAmount) go(s, t, kFast);
                else s = t;
            }
        for (int b = 0; b < kDecayBands; ++b) go(sm.effL[b], tg.effL[b], kFast);
        if (!(sm == lastDesign) && (++designTick >= 4 || sm == tg)) { designTick = 0; designAll(); lastDesign = sm; }
        pdT = sm.pd * 0.001f * fs;
        tDry = std::cos(sm.mix * kPi * 0.5f); tWet = std::sin(sm.mix * kPi * 0.5f); tOut = dbToGain(sm.out);
        for (int b = 0; b < kPostBands; ++b) controlPost(b);
    }
    void designAll() {
        float pv[kParamCount]; std::copy(p, p + kParamCount, pv);
        for (int i = 0; i < kDecayBands * kDFields; ++i) pv[kDecayFirst + i] = sm.dec[i];
        const SpaceParams sp = spaceAt(sm.space);
        const float T0 = sp.decayS * sm.decay / 100.f;
        const float size = sp.sizeMs * 0.001f * fs;
        float dSum = 0;
        for (int l = 0; l < kLines; ++l) { dS[l] = std::min(size * kRatio[l], (float)lineLen - 8.f - 0.006f * fs); dSum += dS[l]; }
        const DecaySolution r = solveDecay(decayShape(pv, sp, sm.bright, sm.effL), T0, fs, dS[kLongest]);
        const float corr = decayCorrection(r, fs);
        for (int l = 0; l < kLines; ++l) designLine(r, fs, dS[l], lflt[l], corr);
        // level: steady-state energy grows with decay time / mean delay; keep the wet level about constant
        const float dAvg = dSum / kLines / fs;
        lateGain = lateTrim * 6.6f * std::sqrt(std::max(dAvg, 0.002f) / std::max(T0, 0.1f)) * (0.75f + 0.5f * sm.dist);
        // character: diffusion, reflections, modulation
        const float c = sm.chr;
        diffG = 0.72f - 0.42f * std::min(1.f, c * 1.6f);
        const float er = std::max(0.f, 1.1f - 0.75f * sm.dist) * (1.f + 1.2f * std::min(1.f, c * 2.f));
        erGain = erTrim * 0.18f * er * (1.f - 0.3f * sm.space);
        modDepth = (0.08f + 12.f * std::max(0.f, c - 0.5f) * std::max(0.f, c - 0.5f)) * 0.001f * fs;
        const float rate = 0.25f + 1.1f * c;
        for (int l = 0; l < kLines; ++l) { lfoInc[l] = rate * (0.62f + 0.06f * l) / fs; modScale[l] = 0.5f + 0.5f * (float)((l * 7) % kLines) / (kLines - 1); }
        // early reflection pattern, stretched to this space and pushed back by distance
        const float erLen = sp.erMs * (1.f + 0.6f * sm.dist) * 0.001f * fs;
        for (int k = 0; k < kTaps; ++k) {
            erT[0][k] = 0.0005f * fs + kErPos[0][k] * erLen; erT[1][k] = 0.0005f * fs + kErPos[1][k] * erLen;
            erG[0][k] = kErSign[0][k] * std::exp(-2.2f * kErPos[0][k]); erG[1][k] = kErSign[1][k] * std::exp(-2.2f * kErPos[1][k]);
        }
        lateT = 0.0005f * fs + erLen * 0.45f + sm.dist * 0.02f * fs;
        // diffusion allpasses: fixed lengths (changing them while playing would click)
        static const float apMs[2][kAp] = { { 1.31f, 2.93f, 5.37f, 8.71f }, { 1.53f, 3.17f, 5.91f, 7.97f } };
        for (int c2 = 0; c2 < 2; ++c2) for (int a = 0; a < kAp; ++a) apD[c2][a] = std::max(1, std::min(apLen - 1, (int)(apMs[c2][a] * 0.001f * fs)));
        airK = 1.f - std::exp(-2.f * kPi * (20000.f - 14000.f * sm.dist) / fs);
        widthS = sm.width;
        tone = rbj(kRbjHighShelf, fs, 5000.f, (sm.bright - 0.5f) * 10.f, 0.6f);
    }
    void syncPost(int b) {
        PostBand& d = post[b];
        d.used = p[pp(b, kPUsed)] > 0.5f; d.on = p[pp(b, kPOn)] > 0.5f;
        d.shape = (int)(p[pp(b, kPShape)] + 0.5f); d.slope = (int)(p[pp(b, kPSlope)] + 0.5f);
    }
    void controlPost(int i) {
        PostBand& b = post[i];
        syncPost(i);
        const float fadeStep = (float)kCtl / (0.008f * fs); const int holdTicks = (int)(0.03f * fs / kCtl);
        const float smooth = 1.f - std::exp(-(float)kCtl / (0.015f * fs));
        const bool structural = b.shape != b.curShape || b.slope != b.curSlope;
        if (structural && (b.mix > 0.f || b.m > 0.f)) b.swapping = true;
        if (structural && b.mix <= 0.f && b.m <= 0.f) { b.curShape = b.shape; b.curSlope = b.slope; b.reset(); b.swapping = false; b.hold = holdTicks; }
        if (b.hold > 0) --b.hold;
        const float target = (b.used && b.on && !b.swapping && b.hold == 0) ? 1.f : 0.f;
        if (b.mix < target) b.mix = std::min(target, b.mix + fadeStep); else if (b.mix > target) b.mix = std::max(target, b.mix - fadeStep);
        if (b.mix <= 0.f && b.m <= 0.f && b.swapping) { b.curShape = b.shape; b.curSlope = b.slope; b.swapping = false; b.reset(); b.hold = holdTicks; }
        b.m = clampf(b.m, 0.f, 1.f); if (std::fabs(b.m - b.mix) < 1e-4f) b.m = b.mix;
        b.dm = (b.mix - b.m) / kCtl;
        if (!b.used) { b.reset(); b.m = b.mix = 0; b.dm = 0; return; }
        const float tOct = std::log2(clampf(p[pp(i, kPFreq)], 10.f, 30000.f)), tG = p[pp(i, kPGain)], tQ = std::log2(clampf(p[pp(i, kPQ)], 0.025f, 40.f));
        if (b.mix <= 0.f && b.hold > 0) { b.oct = tOct; b.gain = tG; b.lq = tQ; }
        else { b.oct += (tOct - b.oct) * smooth; b.gain += (tG - b.gain) * smooth; b.lq += (tQ - b.lq) * smooth; }
        b.design(fs);
    }
    inline float readPre(int c, float d) const {
        const float pos = (float)(prePos + preLen) - clampf(d, 0.f, (float)preLen - 4.f);     // always positive: truncation is floor
        const int i0 = (int)pos; const float t = pos - (float)i0;
        const float a = pre[c][i0 & (preLen - 1)], b = pre[c][(i0 + 1) & (preLen - 1)];
        return a + (b - a) * t;
    }
    inline float readLine(int l, float d) const {                 // cubic (Hermite) interpolation
        const float pos = (float)(linePos + lineLen) - clampf(d, 2.f, (float)lineLen - 4.f);   // always positive: truncation is floor
        const int i1 = (int)pos; const float t = pos - (float)i1; const int M = lineLen - 1;
        const float* b = line[l].data();
        const float y0 = b[(i1 - 1) & M], y1 = b[i1 & M], y2 = b[(i1 + 1) & M], y3 = b[(i1 + 2) & M];
        const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }
    static void hadamard(float* v) {
        for (int h = 1; h < kLines; h <<= 1)
            for (int i = 0; i < kLines; i += h << 1)
                for (int j = i; j < i + h; ++j) { const float a = v[j], b = v[j + h]; v[j] = a + b; v[j + h] = a - b; }
        for (int i = 0; i < kLines; ++i) v[i] *= 0.25f;
    }

    float fs = 48000.f, tempo = 120.f; int designTick = 0;
    std::vector<float> pre[2], line[kLines], ap[2][kAp];
    int preLen = 1, lineLen = 1, apLen = 1, prePos = 0, linePos = 0, apPos = 0, ctl = 0;
    int apD[2][kAp] = {};
    static constexpr float kGlide = 0.004f;             // per-sample glide of delay times (about 5 ms at 48 kHz)
    float dS[kLines] = {}, dCur[kLines] = {}, erS[2][kTaps] = {}, lateS = 0, lfo[kLines] = {}, lfoInc[kLines] = {}, modScale[kLines] = {}, modDepth = 0;
    LineFilter lflt[kLines];
    float fz[kLines][10][2];
    float erT[2][kTaps] = {}, erG[2][kTaps] = {}, lateT = 0, erGain = 0, lateGain = 0, diffG = 0.6f, airK = 1, widthS = 1;
    float air[2] = {}, pdS = 0, pdT = 0, pdSmooth = 0.0015f;
    BQ tone; float toneZ[2][2] = {};
    PostBand post[kPostBands];
    float tDry = 1, tWet = 0, tOut = 1, gDry = 1, gWet = 0, gOut = 1;
    float mIn[2] = {}, mOut[2] = {};
};

} // namespace geode
