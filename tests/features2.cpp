// Geode 2.0 behaviour: decay rate 25-400%, the Vintage and Plate algorithms, Thickness, Ducking, Auto Gate, Freeze,
// per-band channel of the post EQ.
#include "../plugins/Geode/ParamDefs.hpp"
#include "../plugins/Geode/Presets.hpp"
#include <cstdio>
#include <random>
using namespace geode;

static int fails = 0;
#define CHECK(c, ...) do { bool ok_ = (c); char m_[400]; snprintf(m_, sizeof m_, __VA_ARGS__); printf("%s  %s\n", ok_ ? "PASS" : "FAIL", m_); if (!ok_) fails++; } while (0)
static const float FS = 48000.f;
struct Out { std::vector<float> l, r; };
static Out run(Engine& e, const std::vector<float>& inL, const std::vector<float>& inR) {
    Out o; o.l.resize(inL.size()); o.r.resize(inL.size());
    for (size_t i = 0; i < inL.size(); i += 256) { const uint32_t n = (uint32_t)std::min<size_t>(256, inL.size() - i); e.process(&inL[i], &inR[i], &o.l[i], &o.r[i], n); }
    return o;
}
static Engine* make(std::initializer_list<std::pair<uint32_t, float>> ps) {
    Engine* e = new Engine(); e->setSampleRate(FS); e->p[kMix] = 100;
    for (auto& kv : ps) e->p[kv.first] = kv.second;
    e->reset(); return e;
}
static double energy(const std::vector<float>& a, size_t from, size_t to) { double s = 0; for (size_t i = from; i < to && i < a.size(); ++i) s += (double)a[i] * a[i]; return s; }
static double db(double e) { return 10 * std::log10(e + 1e-30); }
static float rt60(const std::vector<float>& h) {
    std::vector<double> sch(h.size() + 1, 0.0);
    for (size_t i = h.size(); i-- > 0;) sch[i] = sch[i + 1] + (double)h[i] * h[i];
    const double tot = sch[0]; size_t a = 0, b = 0;
    for (size_t i = 0; i < h.size(); ++i) { const double d = 10 * std::log10(sch[i] / tot + 1e-30); if (!a && d < -5) a = i; if (d < -35) { b = i; break; } }
    if (!a || !b) return -1;
    return (float)((b - a) / FS * 60.0 / 30.0);
}
static std::vector<float> bandpass(const std::vector<float>& x, float f, float q) {
    const double w = 2 * 3.14159265358979 * f / FS, al = std::sin(w) / (2 * q), a0 = 1 + al;
    const double b0 = al / a0, b2 = -al / a0, a1 = -2 * std::cos(w) / a0, a2 = (1 - al) / a0;
    std::vector<float> y(x.size()); double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    for (size_t i = 0; i < x.size(); ++i) { const double v = b0 * x[i] + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = x[i]; y2 = y1; y1 = v; y[i] = (float)v; }
    return y;
}
static std::vector<float> band3(const std::vector<float>& x, float f, float q) { return bandpass(bandpass(bandpass(x, f, q), f, q), f, q); }
static std::vector<float> impulseResponse(Engine& e, float secs) {
    std::vector<float> in((size_t)(secs * FS), 0.f); in[0] = 1;
    Out o = run(e, in, in);
    std::vector<float> m(in.size()); for (size_t i = 0; i < in.size(); ++i) m[i] = 0.5f * (o.l[i] + o.r[i]);
    return m;
}

int main() {
    std::mt19937 rng(7); std::normal_distribution<float> nd(0.f, 0.1f);
    std::vector<float> noise(96000); for (float& v : noise) v = nd(rng);

    // 1. decay rate from 25% to 400%
    {   Engine* a = make({ { kSpace, 0.4f }, { kDecayRate, 25 } }), * b = make({ { kSpace, 0.4f }, { kDecayRate, 100 } }), * c = make({ { kSpace, 0.4f }, { kDecayRate, 400 } });
        const float ta = rt60(bandpass(impulseResponse(*a, 6.f), 1000.f, 1.f)), tb = rt60(bandpass(impulseResponse(*b, 8.f), 1000.f, 1.f)), tc = rt60(bandpass(impulseResponse(*c, 20.f), 1000.f, 1.f));
        CHECK(ta / tb > 0.2f && ta / tb < 0.32f && tc / tb > 3.4f && tc / tb < 4.6f, "decay rate 25%% / 100%% / 400%%: %.2f / %.2f / %.2f s at 1 kHz (x%.2f, x%.2f; expect x0.25, x4)", ta, tb, tc, ta / tb, tc / tb);
        delete a; delete b; delete c; }

    // 2. algorithms: every one follows the decay time; Vintage is band-limited; Plate is bright, has no early reflections and builds up fast
    {   float rt[kNumAlgorithms], want[kNumAlgorithms], hiRatio[kNumAlgorithms], top[kNumAlgorithms], early[kNumAlgorithms];
        for (int a = 0; a < kNumAlgorithms; ++a) {
            Engine* e = make({ { kSpace, 0.55f }, { kAlgorithm, (float)a }, { kDistance, 20 } });
            const std::vector<float> h = impulseResponse(*e, 10.f);
            rt[a] = rt60(bandpass(h, 1000.f, 1.f)); DecayCurve dc; dc.build(e->p, FS); want[a] = dc.timeAt(1000.f);
            hiRatio[a] = rt60(band3(h, 6000.f, 2.f)) / rt[a];
            Out o = run(*e, noise, noise); top[a] = (float)(db(energy(band3(o.l, 14000.f, 2.f), 48000, 96000)) - db(energy(band3(o.l, 1000.f, 2.f), 48000, 96000)));
            // early energy: first 20 ms of the impulse response, relative to the next 200 ms
            early[a] = (float)(db(energy(h, 0, 960)) - db(energy(h, 960, 10560)));
            delete e;
        }
        bool okRt = true; for (int a = 0; a < kNumAlgorithms; ++a) if (!(rt[a] > want[a] * 0.8f && rt[a] < want[a] * 1.2f)) okRt = false;
        CHECK(okRt, "RT60 at 1 kHz matches the display: Modern %.2f (%.2f), Vintage %.2f (%.2f), Plate %.2f (%.2f) s", rt[0], want[0], rt[1], want[1], rt[2], want[2]);
        CHECK(top[1] < top[0] - 12, "Vintage is band-limited: 14 kHz vs 1 kHz %.1f dB (Modern %.1f dB)", top[1], top[0]);
        float shown[kNumAlgorithms]; for (int a = 0; a < kNumAlgorithms; ++a) { Engine e; e.p[kSpace] = 0.55f; e.p[kAlgorithm] = (float)a; DecayCurve dc; dc.build(e.p, FS); shown[a] = dc.timeAt(6000.f) / dc.timeAt(1000.f); }
        CHECK(hiRatio[2] > hiRatio[0] && shown[2] > shown[0] && shown[1] < shown[0] * 0.75f, "high decay (6 kHz / 1 kHz RT): measured Plate %.2f > Modern %.2f; designed Plate %.2f, Modern %.2f, Vintage %.2f",
              hiRatio[2], hiRatio[0], shown[2], shown[0], shown[1]);
        std::vector<float> dark(noise); { float z = 0; for (float& v : dark) { z += (v - z) * 0.25f; v = z * 2.f; } }      // music-like: most energy low
        float lv[kNumAlgorithms]; for (int a = 0; a < kNumAlgorithms; ++a) { Engine* e = make({ { kSpace, 0.4f }, { kAlgorithm, (float)a } }); Out o = run(*e, dark, dark); lv[a] = (float)db(energy(o.l, 24000, 96000)); delete e; }
        CHECK(std::fabs(lv[1] - lv[0]) < 2.5f && std::fabs(lv[2] - lv[0]) < 2.5f, "wet level on darker noise: Vintage %+.1f dB, Plate %+.1f dB relative to Modern", lv[1] - lv[0], lv[2] - lv[0]);
        CHECK(early[2] < early[0] - 3, "early energy (first 20 ms vs next 200 ms): Plate %.1f dB, Modern %.1f dB", early[2], early[0]); }

    // 3. freeze: the tail holds, new input is kept out, and it decays again when released
    {   Engine* e = make({ { kSpace, 0.3f } }); const int N = (int)FS * 8; std::vector<float> in(N, 0.f), L(N), R(N);
        for (int i = 0; i < 4800; ++i) in[i] = noise[i];
        for (int i = (int)(FS * 3); i < (int)(FS * 3.2f); ++i) in[i] = 4.f * noise[i % 96000];      // loud input while frozen
        for (int i = 0; i < N; i += 256) {
            if (i == 256 * 40) e->p[kFreeze] = 1;            // ~0.2 s
            if (i == 256 * 1125) e->p[kFreeze] = 0;           // 6 s
            e->process(&in[i], &in[i], &L[i], &R[i], std::min(256, N - i)); }
        const double a = db(energy(L, (size_t)(FS * 1), (size_t)(FS * 2))), b = db(energy(L, (size_t)(FS * 4), (size_t)(FS * 5))), c = db(energy(L, (size_t)(FS * 2.9f), (size_t)(FS * 3.0f))) + 10,
                     d = db(energy(L, (size_t)(FS * 3.1f), (size_t)(FS * 3.2f))) + 10, z = db(energy(L, (size_t)(FS * 7.5f), (size_t)(FS * 8)));
        CHECK(std::fabs(a - b) < 3 && std::fabs(d - c) < 1.5 && a - z > 25, "freeze: energy 1-2 s %.1f dB, 4-5 s %.1f dB; loud input while frozen changes it %.2f dB; 1.5 s after release %.0f dB down",
              a, b, d - c, a - z);
        delete e; }

    // 4. thickness: less dynamic range between loud and quiet passages (compression), about the same level
    {   auto range = [&](float t, double& lvl) { Engine* e = make({ { kThickness, t }, { kSpace, 0.2f } }); std::vector<float> in(noise); for (size_t i = 0; i < in.size(); ++i) in[i] *= (i / 12000) % 2 == 0 ? 4.f : 0.4f;
            Out o = run(*e, in, in); double loud = 0, quiet = 0;
            for (int k = 2; k < 8; ++k) (k % 2 == 0 ? loud : quiet) += energy(o.l, k * 12000 + 6000, k * 12000 + 12000);
            lvl = db(energy(o.l, 24000, o.l.size())); delete e; return db(loud) - db(quiet); };
        double l0, l1; const double r0 = range(0, l0), r1 = range(100, l1);
        CHECK(r1 < r0 - 6 && std::fabs(l1 - l0) < 3, "thickness 0 -> 100%%: loud vs quiet passages %.1f -> %.1f dB apart, overall level %+.1f dB", r0, r1, l1 - l0); }

    // 5. ducking: the reverb steps back while the dry signal plays, then returns
    {   auto wetAt = [&](float duck, double& during, double& after) { Engine* e = make({ { kDucking, duck }, { kSpace, 0.6f } }); const int N = (int)FS * 3; std::vector<float> in(N, 0.f);
            for (int i = 0; i < (int)FS; ++i) in[i] = 0.5f * std::sin(2 * kPi * 330 * i / FS);
            Out o = run(*e, in, in); during = db(energy(o.l, 24000, 48000)); after = db(energy(o.l, (size_t)(FS * 1.6f), (size_t)(FS * 1.9f))); delete e; };
        double d0, a0, d1, a1; wetAt(0, d0, a0); wetAt(100, d1, a1);
        CHECK(d0 - d1 > 15 && std::fabs(a0 - a1) < 2, "ducking 100%%: reverb %.1f dB lower while a -6 dBFS tone plays; 0.6-0.9 s after it stops %+.1f dB", d0 - d1, a1 - a0); }

    // 6. auto gate: the tail is cut after the hold time; open again for the next note
    {   auto tails = [&](bool gate, double& t1, double& t2, double& next) { Engine* e = make({ { kAutoGate, gate ? 1.f : 0.f }, { kGateThreshold, -40 }, { kGateHold, 100 }, { kSpace, 0.8f } });
            const int N = (int)FS * 3; std::vector<float> in(N, 0.f); for (int i = 0; i < 2400; ++i) in[i] = 0.5f * noise[i]; for (int i = (int)FS * 2; i < (int)FS * 2 + 2400; ++i) in[i] = 0.5f * noise[i - (int)FS * 2];
            Out o = run(*e, in, in); t1 = db(energy(o.l, 2400, 4800)); t2 = db(energy(o.l, (size_t)(FS * 0.5f), (size_t)(FS * 0.55f))); next = db(energy(o.l, (size_t)(FS * 2) + 2400, (size_t)(FS * 2) + 4800)); delete e; };
        double g1, g2, gn, n1, n2, nn; tails(true, g1, g2, gn); tails(false, n1, n2, nn);
        CHECK(n1 - n2 < 25 && g1 - g2 > 70 && std::fabs(gn - nn) < 3, "auto gate (-40 dB, hold 100 ms): 0.5 s after the hit the tail is %.0f dB down (%.0f dB without the gate); the next hit opens it again (%+.1f dB)",
              g1 - g2, n1 - n2, gn - nn); }

    // 7. post EQ band channels: Left leaves the right alone, Mid leaves the side alone, Side changes only the side
    {   auto render = [&](int place, Out& o) { Engine* e = make({ { pp(0, kPUsed), place >= 0 ? 1.f : 0.f }, { pp(0, kPShape), kPBell }, { pp(0, kPFreq), 1000 }, { pp(0, kPGain), 12 }, { pp(0, kPQ), 1 },
                                                           { placeParam(0), (float)std::max(0, place) } });
            std::vector<float> r(noise.size()); for (size_t i = 0; i < r.size(); ++i) r[i] = noise[(i * 7 + 13) % noise.size()];
            o = run(*e, noise, r); delete e; };
        Out off, left, mid, side, st; render(-1, off); render(kLeftOnly, left); render(kMid, mid); render(kSide, side); render(kStereo, st);
        double eR = 0, eS = 0, eM = 0; for (size_t i = 4800; i < off.l.size(); ++i) { eR = std::max(eR, (double)std::fabs(left.r[i] - off.r[i]));
            eS = std::max(eS, (double)std::fabs((mid.l[i] - mid.r[i]) - (off.l[i] - off.r[i]))); eM = std::max(eM, (double)std::fabs((side.l[i] + side.r[i]) - (off.l[i] + off.r[i]))); }
        std::vector<float> sMid(off.l.size()), sOff(off.l.size()); for (size_t i = 0; i < sMid.size(); ++i) { sMid[i] = mid.l[i] + mid.r[i]; sOff[i] = off.l[i] + off.r[i]; }
        const double boost = db(energy(band3(sMid, 1000.f, 4.f), 24000, 96000)) - db(energy(band3(sOff, 1000.f, 4.f), 24000, 96000));
        CHECK(eR < 1e-6 && eS < 1e-5 && eM < 1e-5 && boost > 10, "post EQ channels: Left-only changes the right by %.1e, Mid-only the side by %.1e, Side-only the mid by %.1e; the mid gets %+.1f dB at 1 kHz",
              eR, eS, eM, boost); }

    // 8. everything at once at 44.1-96 kHz with random settings stays finite and bounded
    {   std::mt19937 g(3); std::uniform_real_distribution<float> u(0, 1); bool ok = true; float worst = 0;
        for (int sr : { 44100, 96000 }) for (int t = 0; t < 6; ++t) {
            Engine e; e.setSampleRate((float)sr);
            for (uint32_t k = 0; k < kParamCount; ++k) { const ParamDef& d = paramDef(k); if (d.hints & kHintOutput) continue; float v = (d.hints & kHintLog) ? d.min * std::pow(d.max / d.min, u(g)) : d.min + (d.max - d.min) * u(g); if (d.hints & kHintInteger) v = std::round(v); e.p[k] = v; }
            e.p[kOutput] = 0; e.reset(); std::vector<float> in(sr * 2); for (size_t i = 0; i < in.size(); ++i) in[i] = 0.5f * noise[i % noise.size()] * 5;
            Out o = run(e, in, in); for (float v : o.l) { if (!std::isfinite(v)) ok = false; worst = std::max(worst, std::fabs(v)); } }
        CHECK(ok && worst < 64, "random 2.0 settings at 44.1 and 96 kHz: finite, peak %.2f", worst); }

    // 9. presets: complete and in range
    {   int bad = 0; for (uint32_t p = 0; p < kNumPresets; ++p) for (uint32_t k = 0; k < kParamCount; ++k) { const ParamDef& d = paramDef(k);
            if (!(d.hints & kHintOutput) && (presets()[p].v[k] < d.min || presets()[p].v[k] > d.max)) ++bad; }
        CHECK(bad == 0, "%u presets, %d values out of range", kNumPresets, bad); }

    printf("%s (%d failures)\n", fails ? "FAILED" : "ALL PASSED", fails);
    return fails;
}
