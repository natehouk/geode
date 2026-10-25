// Geode — impulse response import. Reads a WAV file and works out Geode settings that sound like it: the space and
// decay rate from the mid-band decay time, the Decay Rate EQ from the decay time per octave band, the predelay from the
// gap after the direct sound, the distance from the early-to-late energy balance, the width from the left/right
// correlation, and the Post EQ from the remaining difference in spectrum. This is analysis, not convolution: the
// result is ordinary Geode settings. Generic room-acoustics measures (Schroeder backward integration, ISO 3382 style
// decay fits, clarity C50). License: MIT.
#pragma once
#include "GeodeEngine.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace geode {

// ------------------------------------------------------------------ WAV reading
struct WavData { float fs = 0; int channels = 0; std::vector<float> l, r; };

inline bool parseWav(const uint8_t* d, size_t n, WavData& w, std::string& err) {
    auto u16 = [&](size_t o) { return (uint32_t)d[o] | ((uint32_t)d[o + 1] << 8); };
    auto u32 = [&](size_t o) { return u16(o) | (u16(o + 2) << 16); };
    if (n < 12 || std::memcmp(d, "RIFF", 4) != 0 || std::memcmp(d + 8, "WAVE", 4) != 0) { err = "not a WAV file"; return false; }
    int fmt = 0, ch = 0, bits = 0; uint32_t rate = 0; const uint8_t* data = nullptr; size_t dataLen = 0;
    for (size_t o = 12; o + 8 <= n;) {
        const uint32_t len = u32(o + 4); const size_t body = o + 8, avail = std::min<size_t>(len, n - body);
        if (!std::memcmp(d + o, "fmt ", 4) && avail >= 16) {
            fmt = (int)u16(body); ch = (int)u16(body + 2); rate = u32(body + 4); bits = (int)u16(body + 14);
            if (fmt == 0xFFFE && avail >= 26) fmt = (int)u16(body + 24);          // WAVE_FORMAT_EXTENSIBLE: the sub-format
        } else if (!std::memcmp(d + o, "data", 4)) { data = d + body; dataLen = avail; }
        o = body + len + (len & 1);
    }
    if (!data || ch < 1 || rate < 8000 || rate > 768000) { err = "no audio data, or an unsupported layout"; return false; }
    const bool isFloat = fmt == 3, isPcm = fmt == 1;
    if (!(isPcm && (bits == 16 || bits == 24 || bits == 32)) && !(isFloat && (bits == 32 || bits == 64))) { err = "unsupported sample format (use 16/24/32-bit PCM or 32/64-bit float)"; return false; }
    const int bps = bits / 8; const size_t frames = dataLen / (size_t)(bps * ch);
    if (frames < 64) { err = "the file is too short"; return false; }
    const size_t maxFrames = (size_t)rate * 30;                                     // 30 s is plenty for any reverb
    const size_t nf = std::min(frames, maxFrames);
    w.fs = (float)rate; w.channels = ch; w.l.assign(nf, 0.f); w.r.assign(nf, 0.f);
    auto sample = [&](const uint8_t* p) -> float {
        if (isFloat) { if (bits == 32) { float f; std::memcpy(&f, p, 4); return std::isfinite(f) ? f : 0.f; } double v; std::memcpy(&v, p, 8); return std::isfinite(v) ? (float)v : 0.f; }
        if (bits == 16) return (int16_t)(p[0] | (p[1] << 8)) / 32768.f;
        if (bits == 24) { int32_t v = (int32_t)((uint32_t)p[0] << 8 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 24) >> 8; return v / 8388608.f; }
        int32_t v; std::memcpy(&v, p, 4); return v / 2147483648.f;
    };
    for (size_t i = 0; i < nf; ++i) {
        const uint8_t* f = data + i * (size_t)(bps * ch);
        w.l[i] = sample(f); w.r[i] = ch > 1 ? sample(f + bps) : w.l[i];
    }
    return true;
}
inline bool readWav(const char* path, WavData& w, std::string& err) {
    FILE* f = std::fopen(path, "rb"); if (!f) { err = "could not open the file"; return false; }
    std::vector<uint8_t> buf; uint8_t tmp[65536]; size_t k;
    while ((k = std::fread(tmp, 1, sizeof tmp, f)) > 0) { buf.insert(buf.end(), tmp, tmp + k); if (buf.size() > (size_t)400 << 20) break; }
    std::fclose(f);
    return parseWav(buf.data(), buf.size(), w, err);
}

// ------------------------------------------------------------------ measurement helpers
namespace ir {
struct Biquad { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };
inline Biquad bandpass(float fs, float f, float q) {
    const double w = 2 * 3.14159265358979 * f / fs, al = std::sin(w) / (2 * q), a0 = 1 + al;
    Biquad b; b.b0 = al / a0; b.b1 = 0; b.b2 = -al / a0; b.a1 = -2 * std::cos(w) / a0; b.a2 = (1 - al) / a0; return b;
}
inline void filt(std::vector<float>& x, const Biquad& b) {
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    for (float& v : x) { const double y = b.b0 * v + b.b1 * x1 + b.b2 * x2 - b.a1 * y1 - b.a2 * y2; x2 = x1; x1 = v; y2 = y1; y1 = y; v = (float)y; }
}
// an octave band: three passes of a band-pass, so neighbouring bands barely leak in
inline std::vector<float> octave(const std::vector<float>& x, float fs, float f) { std::vector<float> y(x); const Biquad b = bandpass(fs, f, 1.4f); filt(y, b); filt(y, b); filt(y, b); return y; }
// decay time (RT60) from the Schroeder backward integral, fitted on the part above the noise floor
inline float decayTime(const std::vector<float>& h, float fs, size_t from) {
    if (h.size() <= from + 64) return -1;
    const size_t n = h.size() - from, win = std::max<size_t>(16, (size_t)(0.01f * fs));
    // noise floor: the mean energy of the last tenth
    double floorE = 0; const size_t tail = std::max<size_t>(n / 10, win);
    for (size_t i = h.size() - tail; i < h.size(); ++i) floorE += (double)h[i] * h[i];
    floorE /= tail;
    // where the smoothed envelope meets the floor (+6 dB): integrate only up to there
    size_t cut = h.size(); double peak = 0; size_t peakAt = from;
    for (size_t i = from; i + win < h.size(); i += win) { double e = 0; for (size_t k = i; k < i + win; ++k) e += (double)h[k] * h[k]; e /= win; if (e > peak) { peak = e; peakAt = i; } }
    for (size_t i = peakAt; i + win < h.size(); i += win) { double e = 0; for (size_t k = i; k < i + win; ++k) e += (double)h[k] * h[k]; e /= win; if (e < floorE * 4.0) { cut = i; break; } }
    std::vector<double> sch(cut - from + 1, 0.0);
    for (size_t i = cut; i-- > from;) sch[i - from] = sch[i - from + 1] + std::max(0.0, (double)h[i] * h[i] - floorE);
    const double tot = sch[0]; if (tot <= 0) return -1;
    // least-squares line through the decay curve between -5 dB and -25 dB (-35 dB when there is room)
    const double endDb = 10 * std::log10(sch[sch.size() > 2 ? sch.size() - 2 : 0] / tot + 1e-30) < -38 ? -35 : -25;
    double sx = 0, sy = 0, sxx = 0, sxy = 0; int cnt = 0;
    for (size_t i = 0; i + 1 < sch.size(); i += 4) {
        const double dB = 10 * std::log10(sch[i] / tot + 1e-30);
        if (dB > -5) continue;
        if (dB < endDb) break;
        const double t = i / (double)fs; sx += t; sy += dB; sxx += t * t; sxy += t * dB; ++cnt;
    }
    if (cnt < 8) return -1;
    const double slope = (cnt * sxy - sx * sy) / (cnt * sxx - sx * sx);
    return slope < -1e-3 ? (float)(-60.0 / slope) : -1;
}
inline double energy(const std::vector<float>& x, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b && i < x.size(); ++i) s += (double)x[i] * x[i]; return s; }
// energy in third-octave bands from 50 Hz up, through an FFT of the whole response
static constexpr int kSpecBands = 25;                  // 50 Hz .. 12.7 kHz
inline float specHz(int k) { return 50.f * std::exp2(k / 3.f); }
inline void spectrum(const std::vector<float>& x, size_t from, float fs, float* outDb) {
    int nfft = 1; const size_t len = x.size() > from ? x.size() - from : 0; while ((size_t)nfft < len && nfft < (1 << 21)) nfft <<= 1;
    std::vector<cpx> buf(nfft, cpx(0, 0)); for (size_t i = 0; i < len && (int)i < nfft; ++i) buf[i] = cpx(x[from + i], 0);
    Fft f; f.init(nfft); f.run(buf.data(), false);
    for (int k = 0; k < kSpecBands; ++k) {
        const float lo = specHz(k) * std::exp2(-1.f / 6.f), hi = specHz(k) * std::exp2(1.f / 6.f);
        const int a = std::max(1, (int)(lo / fs * nfft)), b = std::min(nfft / 2, std::max(a + 1, (int)(hi / fs * nfft)));
        double s = 0; for (int i = a; i < b; ++i) s += std::norm(buf[i]);
        outDb[k] = (float)(10 * std::log10(s / (b - a) + 1e-30));
    }
}
} // namespace ir

// ------------------------------------------------------------------ the match
struct IrReport {
    bool directSound = false; float predelayMs = 0, rtMid = 0, distance = 0, width = 0, decayRate = 100; int space = 0;
    static constexpr int kOct = 7; float octHz[kOct], rtIr[kOct], rtMatch[kOct];
    float specErrDb = 0;            // RMS difference of the third-octave spectra after matching (100 Hz - 10 kHz)
    std::string message;
};

// renders Geode's wet impulse response (mono sum) with the given settings
inline std::vector<float> renderIr(const float* params, float fs, float secs, std::vector<float>* rightOut = nullptr) {
    Engine e; std::copy(params, params + kParamCount, e.p);
    e.p[kMix] = 100; e.p[kOutput] = 0; e.p[kFreeze] = 0; e.p[kAutoGate] = 0; e.p[kDucking] = 0; e.p[kPredelaySync] = 0;
    e.setSampleRate(fs); e.reset();
    const size_t n = (size_t)(secs * fs); std::vector<float> in(n, 0.f), L(n), R(n); in[0] = 1;
    for (size_t i = 0; i < n; i += 512) { const uint32_t k = (uint32_t)std::min<size_t>(512, n - i); e.process(&in[i], &in[i], &L[i], &R[i], k); }
    std::vector<float> m(n); for (size_t i = 0; i < n; ++i) m[i] = 0.5f * (L[i] + R[i]);
    if (rightOut) { *rightOut = R; for (size_t i = 0; i < n; ++i) m[i] = L[i]; }   // caller wants L and R separately
    return m;
}
inline size_t onsetOf(const std::vector<float>& x, float rel) {
    float pk = 0; for (float v : x) pk = std::max(pk, std::fabs(v));
    for (size_t i = 0; i < x.size(); ++i) if (std::fabs(x[i]) >= rel * pk) return i;
    return 0;
}
// early-to-late balance after the onset (C50, dB)
inline float clarity(const std::vector<float>& x, size_t on, float fs) {
    const size_t a = on + (size_t)(0.05f * fs); return (float)(10 * std::log10((ir::energy(x, on, a) + 1e-20) / (ir::energy(x, a, x.size()) + 1e-20)));
}

// p: in = the current settings (kept where the IR says nothing), out = the matched settings
inline bool matchIr(const WavData& w, float* p, IrReport& rep) {
    const float fs = w.fs; const size_t n = w.l.size();
    std::vector<float> m(n); for (size_t i = 0; i < n; ++i) m[i] = 0.5f * (w.l[i] + w.r[i]);
    float pk = 0; for (float v : m) pk = std::max(pk, std::fabs(v));
    if (pk < 1e-6f) { rep.message = "the file is silent"; return false; }
    // 1. direct sound and predelay
    const size_t t0 = onsetOf(m, 0.1f), d1 = std::min(n, t0 + (size_t)(0.0015f * fs)), d10 = std::min(n, t0 + (size_t)(0.012f * fs));
    const double eDirect = ir::energy(m, t0 > (size_t)(0.0005f * fs) ? t0 - (size_t)(0.0005f * fs) : 0, d1), eAfter = ir::energy(m, d1, d10);
    rep.directSound = eDirect > 6 * eAfter;
    size_t tailFrom = t0;                               // where Geode's part of the sound starts
    if (rep.directSound) {
        // the reverb starts where the envelope after the direct sound first reaches a third of its later maximum
        const size_t win = std::max<size_t>(8, (size_t)(0.001f * fs)); double mx = 0;
        for (size_t i = d1; i + win < std::min(n, d1 + (size_t)(0.3f * fs)); i += win) mx = std::max(mx, ir::energy(m, i, i + win));
        tailFrom = d1;
        for (size_t i = d1; i + win < n; i += win / 2) if (ir::energy(m, i, i + win) >= mx * 0.1) { tailFrom = i; break; }
    }
    std::vector<float> tail(m.begin() + (long)std::min(tailFrom, n), m.end());
    if (tail.size() < (size_t)(0.05f * fs)) { rep.message = "the reverb tail is too short"; return false; }
    // 2. decay time per octave band
    static const float oct[IrReport::kOct] = { 125, 250, 500, 1000, 2000, 4000, 8000 };
    for (int k = 0; k < IrReport::kOct; ++k) { rep.octHz[k] = oct[k]; rep.rtIr[k] = oct[k] < 0.4f * fs ? ir::decayTime(ir::octave(tail, fs, oct[k]), fs, 0) : -1; }
    // fill bands that could not be measured from their neighbours
    for (int k = 0; k < IrReport::kOct; ++k) if (rep.rtIr[k] <= 0) { float v = -1; for (int d = 1; d < IrReport::kOct && v <= 0; ++d) { if (k - d >= 0 && rep.rtIr[k - d] > 0) v = rep.rtIr[k - d]; else if (k + d < IrReport::kOct && rep.rtIr[k + d] > 0) v = rep.rtIr[k + d]; } rep.rtIr[k] = v; }
    if (rep.rtIr[3] <= 0) { rep.message = "could not measure a decay in this file"; return false; }
    rep.rtMid = std::sqrt(rep.rtIr[2] * rep.rtIr[3]);
    // 3. space and decay rate: the space whose natural decay is closest, then the rate for the exact time
    float best = 1e9f; float sBest = 0.5f;
    for (int k = 0; k <= 200; ++k) { const float s = k / 200.f, t = spaceAt(s).decayS, e = std::fabs(std::log(t / rep.rtMid)); if (e < best) { best = e; sBest = s; } }
    p[kSpace] = sBest; p[kAlgorithm] = kModern; p[kBrightness] = 50; p[kFreeze] = 0;
    for (int b = 0; b < kDecayBands; ++b) { p[dp(b, kDUsed)] = 0; p[dp(b, kDOn)] = 1; p[dp(b, kDAmount)] = 0; }
    for (int b = 0; b < kPostBands; ++b) { p[pp(b, kPUsed)] = 0; p[pp(b, kPOn)] = 1; p[pp(b, kPGain)] = 0; p[placeParam(b)] = kStereo; }
    p[kPredelay] = 0; p[kPredelaySync] = 0;
    { DecayCurve dc; Engine def; std::copy(p, p + kParamCount, def.p); def.p[kDecayRate] = 100; dc.build(def.p, fs);
      const float natMid = std::sqrt(dc.timeAt(500) * dc.timeAt(1000));
      p[kDecayRate] = clampf(100.f * rep.rtMid / natMid, kDecayMin, kDecayMax); }
    rep.space = spaceIndex(p[kSpace]); rep.decayRate = p[kDecayRate];
    // 4. Decay Rate EQ: six bands, solved iteratively against Geode's own decay curve
    static const int shp[kDecayBands] = { kDLowShelf, kDBell, kDBell, kDBell, kDBell, kDHighShelf };
    static const float bf[kDecayBands] = { 160, 380, 900, 2100, 4800, 9000 }, bq[kDecayBands] = { 0.7f, 1.0f, 1.0f, 1.0f, 1.0f, 0.7f };
    static const float evalHz[kDecayBands] = { 110, 380, 900, 2100, 4800, 11000 };
    auto targetRt = [&](float hz) {                       // log-log interpolation of the measured bands
        if (hz <= oct[0]) return rep.rtIr[0];
        for (int k = 0; k + 1 < IrReport::kOct; ++k) if (hz <= oct[k + 1]) { const float t = std::log2(hz / oct[k]); return std::exp2(std::log2(rep.rtIr[k]) + (std::log2(rep.rtIr[k + 1]) - std::log2(rep.rtIr[k])) * t); }
        return rep.rtIr[IrReport::kOct - 1];
    };
    for (int b = 0; b < kDecayBands; ++b) { p[dp(b, kDUsed)] = 1; p[dp(b, kDOn)] = 1; p[dp(b, kDShape)] = (float)shp[b]; p[dp(b, kDFreq)] = bf[b]; p[dp(b, kDQ)] = bq[b]; p[dp(b, kDAmount)] = 0; }
    for (int it = 0; it < 12; ++it) {
        DecayCurve dc; dc.build(p, fs);
        for (int b = 0; b < kDecayBands; ++b) {
            const float want = targetRt(std::min(evalHz[b], 0.4f * fs)), got = dc.timeAt(std::min(evalHz[b], 0.4f * fs));
            if (want > 0 && got > 0) p[dp(b, kDAmount)] = clampf(p[dp(b, kDAmount)] + 0.7f * std::log2(want / got), -2.f, 2.f);
        }
    }
    // then refine against Geode's measured decay (the model leaves out interpolation and modulation losses)
    const float rsecs = std::min(12.f, std::max(2.f, 1.6f * rep.rtIr[0]));
    for (int it = 0; it < 3; ++it) {
        const std::vector<float> g = renderIr(p, fs, rsecs); const size_t on = onsetOf(g, 0.1f);
        float meas[IrReport::kOct];
        for (int k = 0; k < IrReport::kOct; ++k) meas[k] = oct[k] < 0.4f * fs ? ir::decayTime(ir::octave(g, fs, oct[k]), fs, on) : -1;
        auto measAt = [&](float hz) {
            int k = 0; while (k + 1 < IrReport::kOct && oct[k + 1] < hz) ++k;
            if (hz <= oct[0]) return meas[0];
            if (k + 1 >= IrReport::kOct || meas[k + 1] <= 0) return meas[k];
            const float t = std::log2(hz / oct[k]); return meas[k] > 0 ? std::exp2(std::log2(meas[k]) + (std::log2(meas[k + 1]) - std::log2(meas[k])) * t) : meas[k + 1]; };
        for (int b = 0; b < kDecayBands; ++b) {
            const float hz = std::min(evalHz[b], 0.4f * fs), want = targetRt(hz), got = measAt(hz);
            if (want > 0 && got > 0) p[dp(b, kDAmount)] = clampf(p[dp(b, kDAmount)] + 0.8f * std::log2(want / got), -2.f, 2.f);
        }
    }
    for (int b = 0; b < kDecayBands; ++b) if (std::fabs(p[dp(b, kDAmount)]) < 0.04f) { p[dp(b, kDUsed)] = 0; p[dp(b, kDAmount)] = 0; }
    // 5. distance from the early-to-late balance (searched by rendering), then the predelay that lines the onsets up
    const float cIr = clarity(tail, 0, fs);
    { float bestE = 1e9f, bestD = 40;
      for (float d = 0; d <= 100.f; d += 12.5f) {
          p[kDistance] = d; const std::vector<float> g = renderIr(p, fs, 0.6f); const size_t on = onsetOf(g, 0.1f);
          const float e = std::fabs(clarity(g, on, fs) - cIr); if (e < bestE) { bestE = e; bestD = d; } }
      p[kDistance] = bestD; rep.distance = bestD; }
    { const std::vector<float> g = renderIr(p, fs, 0.6f); const size_t onG = onsetOf(g, 0.1f), onIr = rep.directSound ? tailFrom - t0 : t0;     // with a direct sound, the dry signal stands in for it
      // Geode cannot place sound before its own onset: if the IR starts sooner, predelay stays 0
      rep.predelayMs = clampf(((float)onIr - (float)onG) / fs * 1000.f, 0.f, 500.f); p[kPredelay] = rep.predelayMs; }
    // 6. width from the correlation of the late left and right channels
    if (w.channels < 2) { p[kWidth] = 0; rep.width = 0; }
    else {
        auto corrOf = [&](const std::vector<float>& a, const std::vector<float>& b, size_t from) { double ab = 0, aa = 0, bb2 = 0;
            for (size_t i = from; i < a.size() && i < b.size(); ++i) { ab += (double)a[i] * b[i]; aa += (double)a[i] * a[i]; bb2 += (double)b[i] * b[i]; }
            return (float)(ab / std::sqrt(aa * bb2 + 1e-30)); };
        const size_t late = std::min(n, tailFrom + (size_t)(0.08f * fs));
        const float cI = corrOf(w.l, w.r, late);
        float pw[kParamCount]; std::copy(p, p + kParamCount, pw); pw[kWidth] = 100;
        std::vector<float> gr; const std::vector<float> gl = renderIr(pw, fs, 1.f, &gr); const float c100 = corrOf(gl, gr, (size_t)(0.08f * fs));
        p[kWidth] = clampf(100.f * (1.f - cI) / std::max(0.05f, 1.f - c100), 0.f, 120.f); rep.width = p[kWidth];
    }
    // 7. Post EQ: match the remaining spectral difference with six bands
    const float secs = std::min(8.f, std::max(1.f, (float)tail.size() / fs));
    std::vector<float> g = renderIr(p, fs, secs);
    float sIr[ir::kSpecBands], sG[ir::kSpecBands], diff[ir::kSpecBands];
    ir::spectrum(tail, 0, fs, sIr); ir::spectrum(g, onsetOf(g, 0.1f), fs, sG);
    double off = 0; int cnt = 0;
    for (int k = 0; k < ir::kSpecBands; ++k) { diff[k] = sIr[k] - sG[k]; if (ir::specHz(k) >= 200 && ir::specHz(k) <= 4000) { off += diff[k]; ++cnt; } }
    off /= std::max(1, cnt); for (float& d : diff) d -= (float)off;          // the overall level is the Mix's job, not the EQ's
    static const int pshp[kPostBands] = { kPLowShelf, kPBell, kPBell, kPBell, kPBell, kPHighShelf };
    static const float pf[kPostBands] = { 120, 300, 800, 2000, 5000, 10000 }, pq[kPostBands] = { 0.7f, 0.9f, 0.9f, 0.9f, 0.9f, 0.7f };
    static const float pe[kPostBands] = { 63, 300, 800, 2000, 5000, 12500 };
    auto targetDb = [&](float hz) { const float x = 3.f * std::log2(hz / 50.f); const int k = std::max(0, std::min(ir::kSpecBands - 2, (int)x)); const float t = clampf(x - k, 0.f, 1.f); return diff[k] + (diff[k + 1] - diff[k]) * t; };
    float gain[kPostBands] = {};
    for (int it = 0; it < 12; ++it)
        for (int b = 0; b < kPostBands; ++b) {
            if (pe[b] > 0.42f * fs) continue;
            float real = 0; for (int c = 0; c < kPostBands; ++c) real += 20.f * std::log10(std::abs(postResponse(pshp[c], 1, pf[c], gain[c], pq[c], pe[b])));
            gain[b] = clampf(gain[b] + 0.8f * (targetDb(pe[b]) - real), -12.f, 12.f);
        }
    for (int b = 0; b < kPostBands; ++b) {
        p[pp(b, kPShape)] = (float)pshp[b]; p[pp(b, kPFreq)] = pf[b]; p[pp(b, kPQ)] = pq[b]; p[pp(b, kPGain)] = std::round(gain[b] * 10.f) / 10.f; p[pp(b, kPSlope)] = 1;
        p[pp(b, kPUsed)] = std::fabs(gain[b]) >= 0.3f ? 1.f : 0.f; p[pp(b, kPOn)] = 1;
    }
    // 8. how close did it get: decay per band and spectrum of the final settings
    g = renderIr(p, fs, secs);
    for (int k = 0; k < IrReport::kOct; ++k) rep.rtMatch[k] = oct[k] < 0.4f * fs ? ir::decayTime(ir::octave(g, fs, oct[k]), fs, onsetOf(g, 0.1f)) : -1;
    ir::spectrum(g, onsetOf(g, 0.1f), fs, sG);
    double e2 = 0; int c2 = 0; off = 0; cnt = 0;
    for (int k = 0; k < ir::kSpecBands; ++k) if (ir::specHz(k) >= 200 && ir::specHz(k) <= 4000) { off += sIr[k] - sG[k]; ++cnt; }
    off /= std::max(1, cnt);
    for (int k = 0; k < ir::kSpecBands; ++k) if (ir::specHz(k) >= 100 && ir::specHz(k) <= 10000 && ir::specHz(k) < 0.4f * fs) { const double d = sIr[k] - sG[k] - off; e2 += d * d; ++c2; }
    rep.specErrDb = (float)std::sqrt(e2 / std::max(1, c2));
    char b[200]; std::snprintf(b, sizeof b, "IR matched: %s, decay %.0f%%, RT %.2f s, predelay %.0f ms%s", kSpaces[rep.space].name, rep.decayRate, rep.rtMid, rep.predelayMs, rep.directSound ? " (direct sound left to the dry signal)" : "");
    rep.message = b;
    return true;
}

} // namespace geode
