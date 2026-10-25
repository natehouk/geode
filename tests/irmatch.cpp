// IR import: WAV parsing, and matching (1) an impulse response Geode rendered itself with known settings, and
// (2) a synthetic room response (direct sound, a gap, then noise decaying at different rates per band).
#include "../plugins/Geode/ParamDefs.hpp"
#include "../plugins/Geode/IrMatch.hpp"
#include <cstdio>
#include <random>
using namespace geode;

static int fails = 0;
#define CHECK(c, ...) do { bool ok_ = (c); char m_[500]; snprintf(m_, sizeof m_, __VA_ARGS__); printf("%s  %s\n", ok_ ? "PASS" : "FAIL", m_); if (!ok_) fails++; } while (0)

static void put16(std::vector<uint8_t>& v, uint32_t x) { v.push_back(x & 255); v.push_back((x >> 8) & 255); }
static void put32(std::vector<uint8_t>& v, uint32_t x) { put16(v, x & 0xffff); put16(v, x >> 16); }
static std::vector<uint8_t> makeWav(const std::vector<float>& l, const std::vector<float>& r, int ch, int fmt, int bits, int rate, bool extensible = false) {
    std::vector<uint8_t> d; const int bps = bits / 8; const uint32_t dataLen = (uint32_t)(l.size() * ch * bps);
    d.insert(d.end(), { 'R', 'I', 'F', 'F' }); put32(d, 0); d.insert(d.end(), { 'W', 'A', 'V', 'E' });
    d.insert(d.end(), { 'L', 'I', 'S', 'T' }); put32(d, 5); d.insert(d.end(), { 'j', 'u', 'n', 'k', '!', 0 });     // odd-sized chunk before fmt
    d.insert(d.end(), { 'f', 'm', 't', ' ' }); put32(d, extensible ? 40 : 16); put16(d, extensible ? 0xFFFE : fmt); put16(d, ch); put32(d, rate); put32(d, rate * ch * bps); put16(d, ch * bps); put16(d, bits);
    if (extensible) { put16(d, 22); put16(d, bits); put32(d, 3); put16(d, fmt); for (int i = 0; i < 14; ++i) d.push_back(0); }
    d.insert(d.end(), { 'd', 'a', 't', 'a' }); put32(d, dataLen);
    for (size_t i = 0; i < l.size(); ++i) for (int c = 0; c < ch; ++c) {
        const float x = c == 0 ? l[i] : r[i];
        if (fmt == 3) { uint32_t u; std::memcpy(&u, &x, 4); put32(d, u); }
        else if (bits == 16) put16(d, (uint16_t)(int16_t)std::lround(clampf(x, -1, 0.99997f) * 32768.f));
        else if (bits == 24) { const int32_t v = (int32_t)std::lround(clampf(x, -1, 0.9999999f) * 8388608.f); d.push_back(v & 255); d.push_back((v >> 8) & 255); d.push_back((v >> 16) & 255); }
        else put32(d, (uint32_t)(int32_t)std::llround(clampf(x, -1, 0.9999999f) * 2147483648.0));
    }
    const uint32_t riff = (uint32_t)d.size() - 8; std::memcpy(&d[4], &riff, 4);
    return d;
}

int main() {
    // 1. WAV formats
    {   std::vector<float> l(1000), r(1000); for (int i = 0; i < 1000; ++i) { l[i] = 0.5f * std::sin(i * 0.01f); r[i] = -0.25f * std::cos(i * 0.02f); }
        struct C { int ch, fmt, bits; bool ext; float tol; const char* name; } cases[] = { { 2, 1, 16, false, 4e-5f, "16-bit stereo" }, { 2, 1, 24, false, 2e-7f, "24-bit stereo" },
            { 1, 1, 32, false, 1e-8f, "32-bit mono" }, { 2, 3, 32, true, 0.f, "32-bit float, extensible" } };
        int bad = 0; std::string msg;
        for (const C& c : cases) { const std::vector<uint8_t> f = makeWav(l, r, c.ch, c.fmt, c.bits, 44100, c.ext); WavData w; std::string err;
            if (!parseWav(f.data(), f.size(), w, err) || w.fs != 44100 || (int)w.l.size() != 1000 || w.channels != c.ch) { ++bad; msg += std::string(c.name) + " (" + err + ") "; continue; }
            float e = 0; for (int i = 0; i < 1000; ++i) e = std::max(e, std::max(std::fabs(w.l[i] - l[i]), std::fabs(w.r[i] - (c.ch == 2 ? r[i] : l[i])))); if (e > c.tol) { ++bad; msg += std::string(c.name) + " inexact "; } }
        WavData w; std::string err; const uint8_t junk[20] = { 'R', 'I', 'F', 'F' };
        CHECK(bad == 0 && !parseWav(junk, sizeof junk, w, err), "WAV: 16/24/32-bit PCM and 32-bit float (extensible), mono and stereo, odd chunks skipped; junk refused (%s) %s", err.c_str(), msg.c_str()); }

    const float fs = 48000;
    // 2. a Geode impulse response with known settings comes back as matching settings
    {   Engine src; src.p[kSpace] = 0.62f; src.p[kDecayRate] = 140; src.p[kPredelay] = 25; src.p[kDistance] = 30; src.p[kWidth] = 100;
        src.p[dp(0, kDUsed)] = 1; src.p[dp(0, kDShape)] = kDHighShelf; src.p[dp(0, kDFreq)] = 4000; src.p[dp(0, kDAmount)] = -1; src.p[dp(0, kDQ)] = 0.7f;
        src.p[pp(0, kPUsed)] = 1; src.p[pp(0, kPShape)] = kPLowCut; src.p[pp(0, kPFreq)] = 150; src.p[pp(0, kPSlope)] = 2;
        std::vector<float> r; const std::vector<float> l = renderIr(src.p, fs, 8.f, &r);
        WavData w; w.fs = fs; w.channels = 2; w.l = l; w.r = r;
        Engine dst; float p[kParamCount]; std::copy(dst.p, dst.p + kParamCount, p); IrReport rep;
        const bool ok = matchIr(w, p, rep);
        float worst = 0; char rts[300]; size_t m = 0;
        for (int k = 0; k < IrReport::kOct; ++k) { worst = std::max(worst, std::fabs(std::log2(rep.rtMatch[k] / rep.rtIr[k]))); m += snprintf(rts + m, sizeof rts - m, "%.0f Hz %.2f/%.2f  ", rep.octHz[k], rep.rtIr[k], rep.rtMatch[k]); }
        CHECK(ok && worst < 0.2f, "Geode IR: decay per octave (IR/match) %s-> worst %.0f%% off", rts, (std::exp2(worst) - 1) * 100);
        CHECK(ok && std::fabs(rep.predelayMs - 25) < 3 && !rep.directSound && rep.specErrDb < 3 && rep.width > 70,
              "Geode IR: predelay %.1f ms (25), no direct sound found, spectrum within %.1f dB RMS, width %.0f%%, distance %.0f%% (30); %s", rep.predelayMs, rep.specErrDb, rep.width, rep.distance, rep.message.c_str()); }

    // 3. a synthetic room: direct sound at 10 ms, then from 30 ms noise whose decay time falls from 3.0 s in the lows to
    //    2.0 s at 1 kHz and 0.8 s at 8 kHz (built octave by octave), independent in left and right
    {   std::mt19937 g(5); std::normal_distribution<float> nd(0, 1);
        const int N = (int)(fs * 5); std::vector<float> l(N, 0.f), r(N, 0.f);
        auto rtAt = [](float hz) { const float x = std::log2(hz / 1000.f); return hz <= 250 ? 3.0f : (hz <= 1000 ? 2.0f * std::pow(1.5f, -x / 2.f) : 2.0f * std::pow(0.4f, x / 3.f)); };
        for (std::vector<float>* ch : { &l, &r })
            for (float fc = 62.5f; fc < 17000; fc *= 2) {
                std::vector<float> v(N, 0.f); const float rt = rtAt(fc);
                for (int i = (int)(0.03f * fs); i < N; ++i) v[i] = nd(g) * std::pow(10.f, -3.f * (i / fs - 0.03f) / rt);
                v = ir::octave(v, fs, fc); for (int i = 0; i < N; ++i) (*ch)[i] += 0.08f * v[i];
            }
        l[(int)(0.01f * fs)] = r[(int)(0.01f * fs)] = 1.f;        // the direct sound
        WavData w; w.fs = fs; w.channels = 2; w.l = l; w.r = r;
        Engine dst; float p[kParamCount]; std::copy(dst.p, dst.p + kParamCount, p); IrReport rep;
        const bool ok = matchIr(w, p, rep);
        for (int k = 0; k < IrReport::kOct; ++k) printf("   %5.0f Hz: IR %.2f s, match %.2f s\n", rep.octHz[k], rep.rtIr[k], rep.rtMatch[k]);
        const float e250 = rep.rtMatch[1] / 3.0f, e1k = rep.rtMatch[3] / 2.0f, e8k = rep.rtMatch[6] / 0.8f;
        (void)nd;
        CHECK(ok && rep.directSound && std::fabs(rep.predelayMs - 20) < 4 && std::fabs(rep.rtIr[3] - 2.0f) < 0.2f,
              "synthetic room: direct sound found, predelay %.1f ms (20), measured RT at 1 kHz %.2f s (2.0)", rep.predelayMs, rep.rtIr[3]);
        CHECK(ok && std::fabs(e250 - 1) < 0.2f && std::fabs(e1k - 1) < 0.15f && std::fabs(e8k - 1) < 0.25f && rep.width > 60 && rep.width < 110,
              "synthetic room, matched Geode: RT 250 Hz %.2f s (3.0), 1 kHz %.2f s (2.0), 8 kHz %.2f s (0.8); width %.0f%% (uncorrelated channels); %s",
              rep.rtMatch[1], rep.rtMatch[3], rep.rtMatch[6], rep.width, rep.message.c_str());
        // mono file: width 0
        WavData mono = w; mono.channels = 1; mono.r = mono.l; float p2[kParamCount]; std::copy(dst.p, dst.p + kParamCount, p2); IrReport r2m;
        CHECK(matchIr(mono, p2, r2m) && p2[kWidth] == 0, "a mono IR gives width 0 (%.0f)", p2[kWidth]); }

    // 4. silence and junk are refused politely
    {   WavData w; w.fs = fs; w.channels = 1; w.l.assign(48000, 0.f); w.r = w.l; float p[kParamCount] = {}; IrReport rep;
        CHECK(!matchIr(w, p, rep) && !rep.message.empty(), "silent file refused: \"%s\"", rep.message.c_str()); }

    printf("%s (%d failures)\n", fails ? "FAILED" : "ALL PASSED", fails);
    return fails;
}
